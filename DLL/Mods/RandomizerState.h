#pragma once

#include "ConfigData.h"
#include "RVGLStructs.h"
#include <optional>
#include <string>
#include <vector>

namespace Randomizer {

struct ConfigState {
    // Global or class-member variable to hold the active configuration
    std::optional<ConfigData> activeConfig;
    bool useCupDC = true;
};

struct CarRuntimeState {
    // Backing array to guarantee the memory lifetime of the strings
    std::string patchedStrings[49];

    // Persistent storage for the patched pointer table.
    // The pointers in g_VanillaCarPaths must remain valid after the hook returns,
    // so these are static rather than stack-allocated.
    const char* patchedPtrs[49] = {};

    // Car pool snapshot
    // Populated in Hook_LoadCars once the pool is built.
    std::vector<CarInfo> carPool;
    int carCount = 0;
};

struct TrackRuntimeState {
    // Track pool snapshot
    TrackInfo trackInfoBackup[14] = {};
    std::vector<TrackInfo> vanillaTrackPool;
    int trackCount = 0;

    // Flag to enable/disable difficulty manipulation in unlock checks.
    // Enabled only when we do a track check, since the check functions are shared between cars and tracks.
    bool checkingTrackUnlocks = false;
};

struct ProfileRuntimeState {
    bool skipNextProfileLoad = false;
};

struct PhysicsPatchState {
    // Panga / panga slot state
    int spinnerType = 0;
    float spinnerAngVel = 0.0f;
    // Rotor / rotor slot state
    bool flippable = false;
};

struct FileReadState {
    bool checkSpinner = false;
    bool checkFlippable = false;
    bool storeNextFloat = false;
    bool storeNextInt = false;
    bool storeNextBool = false;
};

struct RandomizerContext {
    ConfigState config;
    CarRuntimeState carState;
    TrackRuntimeState trackState;
    ProfileRuntimeState profileState;
    PhysicsPatchState physicsState;
    FileReadState fileState;
};

RandomizerContext& GetRandomizerContext();
ConfigData* GetActiveConfig();

} // namespace Randomizer
