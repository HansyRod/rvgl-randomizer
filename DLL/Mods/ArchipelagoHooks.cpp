#include "ArchipelagoHooks.h"
#include "ArchipelagoClient.h"
#include "CustomUnlocks.h"
#include "RandomizerState.h"

namespace Randomizer {

FnProcessSDLEvents Orig_ProcessSDLEvents = nullptr;
FnMenuUpdateLoop Orig_MenuUpdateLoop = nullptr;
FnInRaceGameLoop Orig_InRaceGameLoop = nullptr;
FnMainGameLoop Orig_MainGameLoop = nullptr;

void Hook_ProcessSDLEvents() {
    Orig_ProcessSDLEvents();

    // Loading screens also pump SDL events. Polling only updates our own state;
    // native availability is refreshed separately by the normal game-state loops.

    const ConfigData* config = GetActiveConfig();
    if (config != nullptr && config->archipelago.has_value() && IsArchipelagoClientEnabled()) {
        PumpArchipelagoClient();
    }
}

void Hook_MenuUpdateLoop() {
    // This frontend game-state update drives both menu callbacks and the gallery.
    // Apply pending unlocks before those callbacks run.
    const ConfigData* config = GetActiveConfig();
    if (config != nullptr && config->archipelago.has_value()) {
        RefreshArchipelagoUnlocks();
    }
    Orig_MenuUpdateLoop();
}

void Hook_InRaceGameLoop() {
    const ConfigData* config = GetActiveConfig();
    if (config != nullptr && config->archipelago.has_value()) {
        RefreshArchipelagoUnlocks();
    }
    Orig_InRaceGameLoop();
}

int Hook_MainGameLoop(int argc, char** argv) {
    const int result = Orig_MainGameLoop(argc, argv);
    const ConfigData* config = GetActiveConfig();
    if (config != nullptr && config->archipelago.has_value()) {
        // Close sockets on the game thread, before process detach takes the loader lock.
        StopArchipelagoClient();
    }
    return result;
}

}
