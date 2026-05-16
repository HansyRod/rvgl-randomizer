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

#ifndef WSWRAP_NO_SSL
#define WSWRAP_NO_SSL
#endif

#ifndef WSWRAP_NO_COMPRESSION
#define WSWRAP_NO_COMPRESSION
#endif

#ifndef _WEBSOCKETPP_CPP11_THREAD_
#define _WEBSOCKETPP_CPP11_THREAD_
#endif

#ifndef _WEBSOCKETPP_CPP11_STL_
#define _WEBSOCKETPP_CPP11_STL_
#endif

#include "apclient.hpp"
#include "Logger.h"

#include <exception>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>

namespace Randomizer {
namespace {

struct ArchipelagoClientState {
    std::mutex mutex;
    ArchipelagoConnectionConfig config;
    std::shared_ptr<APClient> client;
    bool started = false;
    std::unordered_set<std::string> queuedEventKeys;
    std::unordered_set<int64_t> queuedLocationChecks;
};

ArchipelagoClientState& GetArchipelagoClientState() {
    static ArchipelagoClientState state;
    return state;
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

            constexpr int kReceiveAllItems = 7;
            currentState.client->ConnectSlot(
                currentState.config.slotName,
                currentState.config.password,
                kReceiveAllItems
            );
        });

        state.client->set_slot_connected_handler([](const nlohmann::json&) {
            Logger::TimestampLog("[Archipelago] Slot connected.");
        });

        state.client->set_slot_refused_handler([](const std::list<std::string>& errors) {
            for (const std::string& error : errors) {
                Logger::TimestampLogf("[Archipelago] Slot refused: %s", error.c_str());
            }
        });

        state.client->set_socket_error_handler([](const std::string& error) {
            Logger::TimestampLogf("[Archipelago] Socket error: %s", error.c_str());
        });

        state.client->set_items_received_handler([](const std::list<APClient::NetworkItem>& items) {
            Logger::TimestampLogf("[Archipelago] Received %zu item(s).", items.size());
        });

        state.started = true;
        Logger::TimestampLog("[Archipelago] Client created.");
    } catch (const std::exception& ex) {
        state.client.reset();
        state.started = false;
        Logger::TimestampLogf("[Archipelago] Failed to create client: %s", ex.what());
    }
}

void StopArchipelagoClient() {
    ArchipelagoClientState& state = GetArchipelagoClientState();
    std::lock_guard<std::mutex> lock(state.mutex);

    state.client.reset();
    state.started = false;
    state.queuedEventKeys.clear();
    state.queuedLocationChecks.clear();
}

void PumpArchipelagoClient() {
    std::shared_ptr<APClient> client;

    {
        ArchipelagoClientState& state = GetArchipelagoClientState();
        std::lock_guard<std::mutex> lock(state.mutex);

        if (!state.started || state.client == nullptr) {
            return;
        }

        client = state.client;
    }

    client->poll();

    std::list<int64_t> queuedLocations;
    {
        ArchipelagoClientState& state = GetArchipelagoClientState();
        std::lock_guard<std::mutex> lock(state.mutex);
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

    if (!state.queuedEventKeys.insert(eventKey).second) {
        return;
    }

    Logger::TimestampLogf("[Archipelago] Event key queued: %s", eventKey.c_str());
}

bool IsArchipelagoClientEnabled() {
    ArchipelagoClientState& state = GetArchipelagoClientState();
    std::lock_guard<std::mutex> lock(state.mutex);

    return state.config.enabled;
}

} // namespace Randomizer
