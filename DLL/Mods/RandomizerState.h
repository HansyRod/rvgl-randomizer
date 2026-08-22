#pragma once

#include "ConfigData.h"
#include "RVGLStructs.h"
#include <array>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_set>
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

    // Last evaluated custom unlock state for each car. This must remain
    // independent from CarInfo::selectableByPlayer because native physics
    // synchronization can overwrite that field for custom obtain values.
    // The first selectability pass seeds state silently so loading a profile
    // does not replay old unlocks.
    bool checkCarUnlocksPopup = false;
    std::vector<bool> carSelectableState = {};
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

struct ProgressTableRuntimeState {
    int currentPage = 0;
    bool cacheValid = false;
    int cachedTrackCount = -1;
    std::vector<int> stockTrackIndices;
    std::vector<int> customTrackIndices;
};

struct KnockoutRuntimeState {
    bool menuSelectionActive = false;
    bool modeActive = false;
    bool raceActive = false;
    int lapCountMode = 0;
    int eliminationFrequencyLaps = 1;
    int eliminationsPerEvent = 1;
    int knockedOutGhostMode = 1;
    int nextEliminationLap = 1;
    int eliminatedCount = 0;
    int lastRaceClockMs = 0;
    bool playerWon = false;
};

struct ArchipelagoRuntimeState {
    std::mutex itemMutex;
    std::unordered_set<std::string> receivedItems;
};

struct RandomizerContext {
    ConfigState config;
    CarRuntimeState carState;
    ThirtyCarRuntimeState thirtyCarState;
    TrackRuntimeState trackState;
    ProgressTableRuntimeState progressTableState;
    KnockoutRuntimeState knockoutState;
    ArchipelagoRuntimeState archipelagoState;
};

RandomizerContext& GetRandomizerContext();
ConfigData* GetActiveConfig();
bool IsThirtyCarModeEnabled();
bool IsKnockoutModeEnabled();

} // namespace Randomizer
