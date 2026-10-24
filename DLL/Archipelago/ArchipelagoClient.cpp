#include "ArchipelagoClient.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif

#ifndef AP_NO_SCHEMA
#define AP_NO_SCHEMA
#endif

#ifndef _WEBSOCKETPP_CPP11_THREAD_
#define _WEBSOCKETPP_CPP11_THREAD_
#endif

#ifndef _WEBSOCKETPP_CPP11_STL_
#define _WEBSOCKETPP_CPP11_STL_
#endif

#include "apclient.hpp"
#include "ConfigData.h"
#include "CustomUnlocks.h"
#include "ArchipelagoLog.h"
#include "Logger.h"
#include "ProfileHooks.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <exception>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>

namespace Randomizer {
namespace {

struct ArchipelagoClientState {
    // Protects shared state access by public methods and apclientpp callbacks.
    std::mutex mutex;
    // Connection settings and intent; enabled stays true while connecting/reconnecting.
    ArchipelagoConnectionConfig config;
    // Loaded seed metadata: expected slot identity and item/location mappings.
    std::optional<ArchipelagoConfig> archipelagoConfig;
    // Reverse lookup from server item ID to the local unlock name; empty names are filler.
    std::map<int, std::string, std::less<>> itemNames;
    // Processed delivery indices, preventing duplicate item application across reconnects.
    std::unordered_set<int> receivedItemIndices;
    // The first ReceivedItems packet after authentication is the full inventory.
    // apclientpp omits its index when the item list is empty, so track it here too.
    bool awaitingInitialItems = false;
    // Owns the apclientpp instance; the pump holds a shared copy while polling outside the lock.
    std::shared_ptr<APClient> client;
    // Records successful slot authentication and stays true through automatic reconnects.
    bool previouslyConnected = false; // Only initial attempts have a timeout/fallback.
    // Socket-error flag used to advance an initial attempt after poll() returns.
    bool transportFailed = false;
    // apclientpp handles reconnection. This fallback is only for the first attempt
    // when the player omitted a protocol (WSS first, then WS).
    std::string fallbackUri;
    // Start time of the current endpoint's 15-second initial connection attempt.
    std::chrono::steady_clock::time_point attemptStarted;
    // Already-seen gameplay event keys, retained across reconnects to avoid repeated checks/logs.
    std::unordered_set<std::string> queuedEventKeys;
    // Pending location IDs to send when connected; requeued if the send throws.
    std::unordered_set<int64_t> queuedLocationChecks;
};

ArchipelagoClientState& GetArchipelagoClientState() {
    static ArchipelagoClientState state;
    return state;
}

bool IsSlotConnected(const ArchipelagoClientState& state) {
    // Also requires our seed/slot checks, beyond APClient's state: their callbacks
    // disable the connection on rejection before poll() returns to the menu.
    return state.config.enabled && state.client != nullptr &&
        state.client->get_state() == APClient::State::SLOT_CONNECTED;
}

void ResetConnection(ArchipelagoClientState& state) {
    // Callers hold the mutex. Reset only the session; metadata, inventory and
    // queued checks belong to the loaded configuration and survive retries.
    state.client.reset();
    state.previouslyConnected = false;
    state.transportFailed = false;
    state.awaitingInitialItems = false;
    state.fallbackUri.clear();
    state.config.enabled = false;
    state.config.password.clear();
}

// Called by RequestArchipelagoConnection to validate the server address and
// prepare the initial connection URL and optional fallback URL in memory.
// Trims server in place; the caller saves that input to the profile after validation.
bool SetServerAddress(ArchipelagoClientState& state, std::string& server) {
    const size_t first = server.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        LogArchipelagoMessage("Enter a server address.");
        return false;
    }
    server = server.substr(first, server.find_last_not_of(" \t\r\n") - first + 1);
    if (server.size() > 1023 || server.find_first_of(" \t\r\n\\#") != std::string::npos) {
        LogArchipelagoMessage("Invalid server address.");
        return false;
    }

    std::string address = server;
    std::string scheme;
    const size_t schemeEnd = address.find("://");
    if (schemeEnd != std::string::npos) {
        scheme = address.substr(0, schemeEnd);
        std::transform(scheme.begin(), scheme.end(), scheme.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (scheme != "ws" && scheme != "wss") {
            LogArchipelagoMessage("Use ws:// or wss:// for the server protocol.");
            return false;
        }
        address.erase(0, schemeEnd + 3);
    }
    // Give query-only resources a path separator: host?key=value becomes host/?key=value.
    const size_t query = address.find('?');
    if (query != std::string::npos && query < address.find('/')) {
        address.insert(query, "/");
    }
    const std::string authority = address.substr(0, address.find('/'));
    if (authority.empty() || authority.find('@') != std::string::npos) {
        LogArchipelagoMessage("Enter a host without embedded credentials.");
        return false;
    }
    // WebSocket++ already parses hosts, bracketed IPv6, paths and default ports.
    // Its port parser uses atoi, so reject incomplete/non-numeric ports ourselves.
    // Look after ']' for IPv6 so its internal colons are not mistaken for a port.
    const size_t hostEnd = authority.front() == '[' ? authority.find(']') : 0;
    const size_t colon = authority.find(':', hostEnd);
    if (colon != std::string::npos) {
        const std::string port = authority.substr(colon + 1);
        if (port.empty() || port.size() > 5 || port.find_first_not_of("0123456789") != std::string::npos) {
            LogArchipelagoMessage("The server port must be a number from 1 to 65535.");
            return false;
        }
    }
    state.config.serverUri.clear();
    state.fallbackUri.clear();
    // Honor an explicit scheme; otherwise prepare WSS first, with WS as the fallback.
    for (const char* protocol : {"wss", "ws"}) {
        if (!scheme.empty() && scheme != protocol) {
            continue;
        }
        const websocketpp::uri uri(std::string(protocol) + "://" + address);
        if (!uri.get_valid() || uri.get_host().empty()) {
            LogArchipelagoMessage("Invalid server host or port.");
            return false;
        }
        const std::string host = uri.get_host().find(':') == std::string::npos ?
            uri.get_host() : "[" + uri.get_host() + "]";
        // Always include the port: apclientpp would otherwise insert 38281,
        // whereas we need 443 for WSS and 80 for WS when the player omits it.
        const std::string endpoint = std::string(protocol) + "://" + host + ":" +
            std::to_string(uri.get_port()) + uri.get_resource();
        if (state.config.serverUri.empty()) {
            state.config.serverUri = endpoint;
        } else {
            state.fallbackUri = endpoint;
        }
    }
    return true;
}

void AdvanceInitialEndpoint(ArchipelagoClientState& state) {
    state.client.reset();
    state.transportFailed = false;
    if (!state.fallbackUri.empty()) {
        state.config.serverUri = state.fallbackUri;
        state.fallbackUri.clear();
        LogArchipelagoMessagef("Trying %s as %s...",
            state.config.serverUri.c_str(), state.config.slotName.c_str());
    } else {
        ResetConnection(state);
        LogArchipelagoMessage("Connection failed or timed out. Open Archipelago to retry.");
    }
}

void ReceiveArchipelagoItems(const std::list<APClient::NetworkItem>& items) {
    ArchipelagoClientState& state = GetArchipelagoClientState();
    std::lock_guard<std::mutex> lock(state.mutex);

    if (!state.archipelagoConfig.has_value() || !IsSlotConnected(state) ||
        (items.empty() && !state.awaitingInitialItems)) {
        return;
    }

    // An index-zero packet supplies the complete inventory, including starting items.
    // Replace cached ownership only once the server actually supplies that inventory.
    const bool fullInventory = state.awaitingInitialItems || (!items.empty() && items.front().index == 0);
    state.awaitingInitialItems = false;
    if (fullInventory) {
        state.receivedItemIndices.clear();
        ClearArchipelagoItems();
    }

    for (const APClient::NetworkItem& item : items) {
        // insert() returns {iterator, inserted}; second indicates whether this index is new.
        const auto insertionResult = state.receivedItemIndices.insert(item.index);
        const bool isNewDelivery = insertionResult.second;
        // Skip deliveries whose indices were already processed.
        if (!isNewDelivery) {
            continue;
        }

        const auto itemName = state.itemNames.find(item.item);
        if (itemName == state.itemNames.end()) {
            Logger::TimestampLogf("[Archipelago] Ignoring unmapped item ID %lld at index %d.",
                item.item, item.index);
            continue;
        }

        Logger::TimestampLogf("[Archipelago] Received item '%s' at index %d.",
            itemName->second.empty() ? "Filler" : itemName->second.c_str(), item.index);

        // The empty item string represents filler, which has no unlock effect.
        if (itemName->second.empty()) {
            continue;
        }

        MarkArchipelagoItemReceived(itemName->second);
    }
    SaveArchipelagoProfileReceivedItems(state.config.seed, state.archipelagoConfig->slotNumber);
}

bool HasConnectionDetails(const ArchipelagoConnectionConfig& config) {
    return !config.serverUri.empty() &&
           !config.slotName.empty() &&
           !config.gameName.empty() &&
           !config.uuid.empty();
}

void RecordLocationCheck(ArchipelagoClientState& state, int64_t locationId) {
    // The caller holds the client mutex. Save the completed ID before queueing
    // it for submission so an interrupted send can be retried after restart.
    if (!state.archipelagoConfig.has_value() || locationId <= 0) {
        return;
    }
    const auto& locations = state.archipelagoConfig->locations;
    if (std::none_of(locations.begin(), locations.end(),
        [locationId](const auto& location) { return location.second == locationId; })) {
        return;
    }
    RecordArchipelagoProfileLocationCheck(state.config.seed, state.archipelagoConfig->slotNumber, locationId);
    state.queuedLocationChecks.insert(locationId);
}

std::list<int64_t> TakeQueuedLocationChecks(ArchipelagoClientState& state) {
    std::list<int64_t> locations;

    if (state.queuedLocationChecks.empty()) {
        return locations;
    }

    locations.assign(
        state.queuedLocationChecks.begin(),
        state.queuedLocationChecks.end()
    );
    // Clear only the outgoing batch. Unconfirmed completions remain saved
    // if this send is interrupted or the process exits before the server receives it.
    state.queuedLocationChecks.clear();

    return locations;
}

void SendQueuedLocationChecks(const std::shared_ptr<APClient>& client, const std::list<int64_t>& locations) {
    if (client == nullptr || locations.empty()) {
        return;
    }

    client->LocationChecks(locations);
}

} // anonymous namespace

void ConfigureArchipelagoClient(const ArchipelagoConnectionConfig& config) {
    ArchipelagoClientState& state = GetArchipelagoClientState();
    std::lock_guard<std::mutex> lock(state.mutex);

    state.config = config;
}

void SetArchipelagoClientUUID(const std::string& uuid) {
    ArchipelagoClientState& state = GetArchipelagoClientState();
    std::lock_guard<std::mutex> lock(state.mutex);

    state.config.uuid = uuid;
}

// Called when the player chooses Connect in the Archipelago menu, or when a
// saved profile requests an automatic connection on startup.
bool RequestArchipelagoConnection(const std::string& server, const std::string& password) {
    ArchipelagoClientState& state = GetArchipelagoClientState();
    std::lock_guard<std::mutex> lock(state.mutex);
    // Ignore duplicate requests while connecting or already connected.
    if (state.config.enabled) {
        return false;
    }
    // Require the loaded slot metadata and profile identity before connecting.
    if (!state.archipelagoConfig.has_value() || state.config.uuid.empty() || state.config.slotName.empty()) {
        LogArchipelagoMessage("Load a valid player profile and Archipelago metadata before connecting.");
        return false;
    }
    if (password.size() > 255) {
        LogArchipelagoMessage("Password is too long.");
        return false;
    }
    // Clear the previous session, then validate the address and prepare its endpoints.
    std::string normalizedServer = server;
    ResetConnection(state);
    if (!SetServerAddress(state, normalizedServer)) {
        return false;
    }
    // Persist the server and password-required flag; keep the password in memory only.
    SaveArchipelagoProfileConnection(normalizedServer, !password.empty());
    state.config.password = password;
    // Accept the request. PumpArchipelagoClient creates the client later on the
    // game thread; authentication has not completed when this function returns.
    state.config.enabled = true;
    LogArchipelagoMessagef("Connecting to %s as %s...",
        normalizedServer.c_str(), state.config.slotName.c_str());
    return true;
}

void CancelArchipelagoConnection() {
    ArchipelagoClientState& state = GetArchipelagoClientState();
    std::lock_guard<std::mutex> lock(state.mutex);
    const bool wasEnabled = state.config.enabled;
    ResetConnection(state);
    if (wasEnabled) {
        LogArchipelagoMessage("Connection cancelled.");
    }
}

bool IsArchipelagoSlotConnected() {
    ArchipelagoClientState& state = GetArchipelagoClientState();
    std::lock_guard<std::mutex> lock(state.mutex);
    return IsSlotConnected(state);
}

bool IsArchipelagoConnectionPending() {
    ArchipelagoClientState& state = GetArchipelagoClientState();
    std::lock_guard<std::mutex> lock(state.mutex);
    // enabled stays true during apclientpp's automatic reconnect, even while its
    // socket is disconnected. Keep the form locked until authenticated or cancelled.
    return state.config.enabled && !IsSlotConnected(state);
}

void InitializeArchipelagoMappings(const ArchipelagoConfig* config) {
    ArchipelagoClientState& state = GetArchipelagoClientState();
    std::lock_guard<std::mutex> lock(state.mutex);

    state.archipelagoConfig.reset();
    state.itemNames.clear();
    state.receivedItemIndices.clear();
    state.queuedEventKeys.clear();
    state.queuedLocationChecks.clear();
    ClearArchipelagoItems();

    if (config == nullptr) {
        return;
    }

    state.archipelagoConfig = *config;
    for (const auto& item : config->items) {
        state.itemNames.emplace(item.second, item.first);
    }

    Logger::TimestampLogf("[Archipelago] Loaded %zu location and %zu item mappings.",
        config->locations.size(), state.itemNames.size());
}

std::optional<std::string> GetArchipelagoItemName(int itemId) {
    ArchipelagoClientState& state = GetArchipelagoClientState();
    std::lock_guard<std::mutex> lock(state.mutex);

    const auto item = state.itemNames.find(itemId);
    if (item == state.itemNames.end()) {
        return std::nullopt;
    }

    return item->second;
}

void StartArchipelagoClient() {
    ArchipelagoClientState& state = GetArchipelagoClientState();
    std::lock_guard<std::mutex> lock(state.mutex);

    if (!state.config.enabled) {
        Logger::TimestampLog("[Archipelago] Client disabled.");
        return;
    }

    if (!HasConnectionDetails(state.config)) {
        Logger::TimestampLog("[Archipelago] Client enabled but connection details are incomplete.");
        LogArchipelagoMessage("Open Archipelago after loading a player profile.");
        ResetConnection(state);
        return;
    }

    if (state.client != nullptr) {
        return;
    }

    try {
        state.client = std::make_shared<APClient>(
            state.config.uuid,
            state.config.gameName,
            state.config.serverUri
        );

        state.client->set_room_info_handler([]() {
            ArchipelagoClientState& currentState = GetArchipelagoClientState();
            std::lock_guard<std::mutex> lock(currentState.mutex);

            if (currentState.client == nullptr) {
                return;
            }
            if (currentState.client->has_password()) {
                MarkArchipelagoProfilePasswordRequired();
            }

            if (currentState.client->get_seed() != currentState.config.seed) {
                LogArchipelagoMessage("Room seed does not match the loaded configuration. Connection stopped.");
                currentState.config.enabled = false;
                return;
            }

            constexpr int kReceiveAllItems = 7;
            currentState.client->ConnectSlot(
                currentState.config.slotName,
                currentState.config.password,
                kReceiveAllItems
            );
        });

        state.client->set_slot_connected_handler([](const nlohmann::json&) {
            ArchipelagoClientState& currentState = GetArchipelagoClientState();
            std::lock_guard<std::mutex> lock(currentState.mutex);
            if (!currentState.archipelagoConfig.has_value() ||
                currentState.client->get_player_number() != currentState.archipelagoConfig->slotNumber) {
                LogArchipelagoMessage("Connected slot does not match the loaded configuration. Connection stopped.");
                currentState.config.enabled = false;
                return;
            }
            currentState.previouslyConnected = true;
            currentState.awaitingInitialItems = true;
            const auto completedLocations = GetArchipelagoProfileLocationChecks(
                currentState.config.seed, currentState.archipelagoConfig->slotNumber);
            for (const auto& location : currentState.archipelagoConfig->locations) {
                if (completedLocations.count(location.second) != 0) {
                    currentState.queuedLocationChecks.insert(location.second);
                }
            }
            // Connected replaces apclientpp's checked set with the server's list.
            // LocationChecks later adds IDs to that set before server acknowledgement,
            // so only this fresh snapshot is safe for removing checks from disk.
            const auto confirmedLocations = currentState.client->get_checked_locations();
            for (int64_t locationId : confirmedLocations) {
                currentState.queuedLocationChecks.erase(locationId);
            }
            RemoveConfirmedArchipelagoProfileLocationChecks(currentState.config.seed,
                currentState.archipelagoConfig->slotNumber, confirmedLocations);
            LogArchipelagoMessage("Slot connected.");
        });

        state.client->set_slot_refused_handler([](const std::list<std::string>& errors) {
            for (const std::string& error : errors) {
                LogArchipelagoMessagef("Slot refused: %s", error.c_str());
            }
            ArchipelagoClientState& currentState = GetArchipelagoClientState();
            std::lock_guard<std::mutex> lock(currentState.mutex);
            currentState.config.enabled = false;
        });

        state.client->set_socket_disconnected_handler([]() {
            LogArchipelagoMessage("Disconnected; the client will retry.");
        });

        state.client->set_socket_error_handler([](const std::string& error) {
            LogArchipelagoMessagef("Socket error: %s", error.c_str());
            ArchipelagoClientState& currentState = GetArchipelagoClientState();
            std::lock_guard<std::mutex> lock(currentState.mutex);
            currentState.transportFailed = true;
        });

        state.client->set_items_received_handler([](const std::list<APClient::NetworkItem>& items) {
            Logger::TimestampLogf("[Archipelago] Received %zu item(s).", items.size());
            ReceiveArchipelagoItems(items);
        });

        state.client->set_print_handler([](const std::string& message) {
            LogArchipelagoMessage(message.c_str());
        });

        state.client->set_print_json_handler([](const std::list<APClient::TextNode>& nodes) {
            ArchipelagoClientState& currentState = GetArchipelagoClientState();
            std::lock_guard<std::mutex> lock(currentState.mutex);
            if (currentState.client == nullptr) {
                return;
            }
            // Resolve player, item, and location IDs using the server's data
            // packages, including items belonging to games other than RVGL.
            const std::string message = currentState.client->render_json(nodes);
            LogArchipelagoMessage(message.c_str());
        });

        state.attemptStarted = std::chrono::steady_clock::now();
        Logger::TimestampLog("[Archipelago] Client created.");
    } catch (const std::exception& ex) {
        LogArchipelagoMessagef("Failed to create client: %s", ex.what());
        AdvanceInitialEndpoint(state);
    }
}

void StopArchipelagoClient() {
    ArchipelagoClientState& state = GetArchipelagoClientState();
    std::lock_guard<std::mutex> lock(state.mutex);

    // The main game loop calls this before DLL detach; retry failed disk writes
    // while filesystem access is still safe, before clearing the in-memory queue.
    FlushArchipelagoProfileData();
    ResetConnection(state);
    state.archipelagoConfig.reset();
    state.itemNames.clear();
    state.receivedItemIndices.clear();
    state.queuedEventKeys.clear();
    state.queuedLocationChecks.clear();
    ClearArchipelagoItems();
}

// Runs each frame on the game thread through the SDL event hook while enabled.
// Processes server communication and connection/reconnection progress, then
// submits queued location checks once the slot is authenticated.
void PumpArchipelagoClient() {
    bool needsClient;
    {
        ArchipelagoClientState& state = GetArchipelagoClientState();
        std::lock_guard<std::mutex> lock(state.mutex);
        needsClient = state.config.enabled && state.client == nullptr;
    }
    // Construction is deferred until the game thread runs, outside DllMain.
    // A request or protocol fallback simply leaves the enabled connection without a client.
    if (needsClient) {
        StartArchipelagoClient();
    }

    std::shared_ptr<APClient> client;

    {
        ArchipelagoClientState& state = GetArchipelagoClientState();
        std::lock_guard<std::mutex> lock(state.mutex);

        // Construction may have failed or ended the attempt, leaving no client to poll.
        // An existing client can be polled before connecting; poll() advances that process.
        if (state.client == nullptr) {
            return;
        }

        client = state.client;
    }

    try {
        // Process incoming server messages and advance connection/reconnection work.
        // poll() invokes our handlers synchronously, and those handlers take the
        // client mutex. The shared pointer keeps the client alive without holding the mutex.
        client->poll();
    } catch (const std::exception& ex) {
        LogArchipelagoMessagef("Polling failed: %s", ex.what());
        CancelArchipelagoConnection();
        return;
    }

    std::list<int64_t> queuedLocations;
    {
        ArchipelagoClientState& state = GetArchipelagoClientState();
        std::lock_guard<std::mutex> lock(state.mutex);
        if (!state.config.enabled) {
            // Callbacks can reject a room or slot. Destroy the socket after poll returns.
            ResetConnection(state);
            return;
        }
        if (!state.previouslyConnected && (state.transportFailed ||
            std::chrono::steady_clock::now() - state.attemptStarted >= std::chrono::seconds(15))) {
            // AP callbacks run inside poll(). Replacing its socket must wait until it returns.
            AdvanceInitialEndpoint(state);
            return;
        }
        // Keep completed gameplay checks queued until slot authentication succeeds.
        if (!IsSlotConnected(state)) {
            return;
        }
        // Take the Archipelago location IDs completed in-game and awaiting submission.
        // Copy them into this batch and clear the shared queue while holding its lock.
        queuedLocations = TakeQueuedLocationChecks(state);
    }

    try {
        // Report those completed location IDs to the server through apclientpp's LocationChecks.
        SendQueuedLocationChecks(client, queuedLocations);
    } catch (const std::exception& ex) {
        LogArchipelagoMessagef("Could not send location checks: %s", ex.what());
        ArchipelagoClientState& state = GetArchipelagoClientState();
        std::lock_guard<std::mutex> lock(state.mutex);
        // Restore the batch so a later update can retry a failed submission.
        state.queuedLocationChecks.insert(queuedLocations.begin(), queuedLocations.end());
    }
}

void QueueArchipelagoEventKey(const std::string& eventKey) {
    if (eventKey.empty()) {
        return;
    }

    ArchipelagoClientState& state = GetArchipelagoClientState();
    std::lock_guard<std::mutex> lock(state.mutex);

    if (!state.archipelagoConfig.has_value() || !state.queuedEventKeys.insert(eventKey).second) {
        return;
    }

    const auto location = state.archipelagoConfig->locations.find(eventKey);
    if (location == state.archipelagoConfig->locations.end()) {
        Logger::TimestampLogf("[Archipelago] Ignoring unmapped event key: %s", eventKey.c_str());
        return;
    }

    // The client mutex is already held; the shared recording helper must not lock it again.
    RecordLocationCheck(state, location->second);
    Logger::TimestampLogf("[Archipelago] Event key queued: %s -> location %d", eventKey.c_str(), location->second);
}

bool IsArchipelagoClientEnabled() {
    ArchipelagoClientState& state = GetArchipelagoClientState();
    std::lock_guard<std::mutex> lock(state.mutex);

    return state.config.enabled;
}

} // namespace Randomizer
