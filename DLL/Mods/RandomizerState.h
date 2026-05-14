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

    // Number of cars per race
    int carsPerRace = 0;
};

struct TrackRuntimeState {
    // Track pool snapshot
    TrackInfo trackInfoBackup[14] = {};
    std::vector<TrackInfo> vanillaTrackPool;
    std::vector<TrackInfo> customTrackPool;
    int trackCount = 0;
};

struct RandomizerContext {
    ConfigState config;
    CarRuntimeState carState;
    TrackRuntimeState trackState;
};

RandomizerContext& GetRandomizerContext();
ConfigData* GetActiveConfig();

} // namespace Randomizer
