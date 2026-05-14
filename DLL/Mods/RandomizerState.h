#pragma once

#include "ConfigData.h"
#include "RVGLStructs.h"
#include <array>
#include <optional>
#include <string>
#include <vector>

namespace Randomizer {

constexpr int randomizerMinCarCount = 2;
constexpr int vanillaMaxCarCount = 16;
constexpr int randomizerMaxCarCount = 30;

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

struct ThirtyCarRuntimeState {
    bool cacheValid = false;
    bool participantsExpanded = false;
    bool gridApplied = false;
    bool playersMovedToBack = false;
    int originalParticipantCount = 0;
    std::array<int, randomizerMaxCarCount> generatedModelIds = {};
    std::array<int, randomizerMaxCarCount> runtimeCarIds = {};

    ThirtyCarRuntimeState() {
        generatedModelIds.fill(-1);
        runtimeCarIds.fill(-1);
    }
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
    ThirtyCarRuntimeState thirtyCarState;
    TrackRuntimeState trackState;
};

RandomizerContext& GetRandomizerContext();
ConfigData* GetActiveConfig();

} // namespace Randomizer
