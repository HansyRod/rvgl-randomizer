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
    std::mutex mutex;
    ArchipelagoConnectionConfig config;
    std::optional<ArchipelagoConfig> archipelagoConfig;
    std::map<int, std::string, std::less<>> itemNames;
    std::unordered_set<int> receivedItemIndices;
    std::shared_ptr<APClient> client;
    bool started = false;
    bool startPending = false;
    bool slotConnected = false;
    std::unordered_set<std::string> queuedEventKeys;
    std::unordered_set<int64_t> queuedLocationChecks;
};

ArchipelagoClientState& GetArchipelagoClientState() {
    static ArchipelagoClientState state;
    return state;
}

void ReceiveArchipelagoItems(const std::list<APClient::NetworkItem>& items) {
    ArchipelagoClientState& state = GetArchipelagoClientState();
    std::lock_guard<std::mutex> lock(state.mutex);

    if (!state.archipelagoConfig.has_value() || !state.config.enabled || !state.slotConnected || items.empty()) {
        return;
    }

    // An index-zero packet supplies the complete inventory, including starting items.
    if (items.front().index == 0) {
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
            LogArchipelagoMessagef("[Archipelago] Ignoring unmapped item ID %lld at index %d.",
                item.item, item.index);
            continue;
        }

        LogArchipelagoMessagef("[Archipelago] Received item '%s' at index %d.",
            itemName->second.empty() ? "Filler" : itemName->second.c_str(), item.index);

        // The empty item string represents filler, which has no unlock effect.
        if (itemName->second.empty()) {
            continue;
        }

        MarkArchipelagoItemReceived(itemName->second);
    }
}

bool HasConnectionDetails(const ArchipelagoConnectionConfig& config) {
    return !config.serverUri.empty() &&
           !config.slotName.empty() &&
           !config.gameName.empty() &&
           !config.uuid.empty();
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
    state.startPending = config.enabled && !config.uuid.empty();
}

void SetArchipelagoClientUUID(const std::string& uuid) {
    ArchipelagoClientState& state = GetArchipelagoClientState();
    std::lock_guard<std::mutex> lock(state.mutex);

    if (state.config.uuid == uuid) {
        return;
    }

    // A profile change uses that profile's client identity on the next connection.
    state.client.reset();
    state.started = false;
    state.slotConnected = false;
    state.config.uuid = uuid;
    state.startPending = state.config.enabled && !uuid.empty();
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

    LogArchipelagoMessagef("[Archipelago] Loaded %zu location and %zu item mappings.",
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
        LogArchipelagoMessage("[Archipelago] Client disabled.");
        return;
    }

    if (!HasConnectionDetails(state.config)) {
        LogArchipelagoMessage("[Archipelago] Client enabled but connection details are incomplete.");
        LogArchipelagoMessage("[Archipelago] Set RVGL_AP_SERVER and load a player profile before connecting.");
        state.config.enabled = false;
        return;
    }

    if (state.client != nullptr) {
        state.started = true;
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
            currentState.slotConnected = false;

            if (currentState.client->get_seed() != currentState.config.seed) {
                LogArchipelagoMessage("[Archipelago] Room seed does not match the loaded configuration. Connection stopped.");
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
                LogArchipelagoMessage("[Archipelago] Connected slot does not match the loaded configuration. Connection stopped.");
                currentState.config.enabled = false;
                return;
            }
            currentState.slotConnected = true;
            LogArchipelagoMessage("[Archipelago] Slot connected.");
        });

        state.client->set_slot_refused_handler([](const std::list<std::string>& errors) {
            for (const std::string& error : errors) {
                LogArchipelagoMessagef("[Archipelago] Slot refused: %s", error.c_str());
            }
            ArchipelagoClientState& currentState = GetArchipelagoClientState();
            std::lock_guard<std::mutex> lock(currentState.mutex);
            currentState.config.enabled = false;
        });

        state.client->set_socket_disconnected_handler([]() {
            ArchipelagoClientState& currentState = GetArchipelagoClientState();
            std::lock_guard<std::mutex> lock(currentState.mutex);
            currentState.slotConnected = false;
            LogArchipelagoMessage("[Archipelago] Disconnected; the client will retry.");
        });

        state.client->set_socket_error_handler([](const std::string& error) {
            LogArchipelagoMessagef("[Archipelago] Socket error: %s", error.c_str());
        });

        state.client->set_items_received_handler([](const std::list<APClient::NetworkItem>& items) {
            LogArchipelagoMessagef("[Archipelago] Received %zu item(s).", items.size());
            ReceiveArchipelagoItems(items);
        });

        state.client->set_print_handler([](const std::string& message) {
            LogArchipelagoMessage(("[Archipelago] " + message).c_str());
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
            LogArchipelagoMessage(("[Archipelago] " + message).c_str());
        });

        state.started = true;
        LogArchipelagoMessage("[Archipelago] Client created.");
    } catch (const std::exception& ex) {
        state.client.reset();
        state.started = false;
        state.config.enabled = false;
        LogArchipelagoMessagef("[Archipelago] Failed to create client: %s", ex.what());
    }
}

void StopArchipelagoClient() {
    ArchipelagoClientState& state = GetArchipelagoClientState();
    std::lock_guard<std::mutex> lock(state.mutex);

    state.client.reset();
    state.started = false;
    state.startPending = false;
    state.slotConnected = false;
    state.config.enabled = false;
    state.archipelagoConfig.reset();
    state.itemNames.clear();
    state.receivedItemIndices.clear();
    state.queuedEventKeys.clear();
    state.queuedLocationChecks.clear();
    ClearArchipelagoItems();
}

void PumpArchipelagoClient() {
    bool startPending;
    {
        ArchipelagoClientState& state = GetArchipelagoClientState();
        std::lock_guard<std::mutex> lock(state.mutex);
        startPending = state.startPending;
        state.startPending = false;
    }
    // Construction is deferred until the game thread runs, outside DllMain.
    if (startPending) {
        StartArchipelagoClient();
    }

    std::shared_ptr<APClient> client;

    {
        ArchipelagoClientState& state = GetArchipelagoClientState();
        std::lock_guard<std::mutex> lock(state.mutex);

        if (!state.started || state.client == nullptr) {
            return;
        }

        client = state.client;
    }

    try {
        client->poll();
    } catch (const std::exception& ex) {
        LogArchipelagoMessagef("[Archipelago] Polling failed: %s", ex.what());
        StopArchipelagoClient();
        return;
    }

    std::list<int64_t> queuedLocations;
    {
        ArchipelagoClientState& state = GetArchipelagoClientState();
        std::lock_guard<std::mutex> lock(state.mutex);
        if (!state.config.enabled) {
            // Callbacks can reject a room or slot. Destroy the socket after poll returns.
            state.client.reset();
            state.started = false;
            state.slotConnected = false;
            return;
        }
        if (!state.slotConnected || client->get_state() != APClient::State::SLOT_CONNECTED) {
            return;
        }
        queuedLocations = TakeQueuedLocationChecks(state);
    }

    SendQueuedLocationChecks(client, queuedLocations);
}

void QueueArchipelagoLocationCheck(int64_t locationId) {
    if (locationId <= 0) {
        return;
    }

    ArchipelagoClientState& state = GetArchipelagoClientState();
    std::lock_guard<std::mutex> lock(state.mutex);

    state.queuedLocationChecks.insert(locationId);
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
        LogArchipelagoMessagef("[Archipelago] Ignoring unmapped event key: %s", eventKey.c_str());
        return;
    }

    // The client mutex is already held, so insert directly rather than locking again.
    state.queuedLocationChecks.insert(location->second);
    LogArchipelagoMessagef("[Archipelago] Event key queued: %s -> location %d", eventKey.c_str(), location->second);
}

bool IsArchipelagoClientEnabled() {
    ArchipelagoClientState& state = GetArchipelagoClientState();
    std::lock_guard<std::mutex> lock(state.mutex);

    return state.config.enabled;
}

} // namespace Randomizer
