#include "ArchipelagoHooks.h"
#include "ArchipelagoClient.h"
#include "RandomizerState.h"

namespace Randomizer {

FnMainGameLoop Orig_MainGameLoop = nullptr;
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
