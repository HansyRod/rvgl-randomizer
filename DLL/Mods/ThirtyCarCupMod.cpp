#include "ThirtyCarCupMod.h"
#include "30CarMod.h"
#include "CupOpponentGrid.h"
#include "ExtendedCupResults.h"
#include "ExtendedCupStandingsTable.h"
#include "ProgressTableHooks.h"
#include "RaceInitHooks.h"
#include "RandomizerState.h"
#include "RVGLFunctions.h"
#include "RVGLMemory.h"
#include "TrackHooks.h"
#include <algorithm>
#include <array>
#include <cstdio>

namespace Randomizer {

FnBuildGrid Orig_BuildGrid = nullptr;
FnUpdateCupPostRaceProgress Orig_UpdateCupPostRaceProgress = nullptr;
FnDrawCupStandingsTable Orig_DrawCupStandingsTable = nullptr;

namespace {

constexpr int kMaxCupCars = randomizerMaxCarCount;
constexpr int kNativeCupCars = vanillaMaxCarCount;
constexpr int kCpuRaceCarState = 3;
constexpr int kPlayerRaceCarState = 1;

struct ThirtyCarCupState {
    bool active = false;
    bool rosterGenerated = false;
    int selectedCupIndex = -1;
    bool gridApplied = false;
    bool playerMovedToBack = false;
    CupProfile* activeCup = nullptr;
    const RandomizedCup* cupConfig = nullptr;
    ExtendedCupResultsState results = {};
    std::array<int, kMaxCupCars> runtimeCarIds = {};

    void Reset() {
        active = false;
        rosterGenerated = false;
        selectedCupIndex = -1;
        gridApplied = false;
        playerMovedToBack = false;
        activeCup = nullptr;
        cupConfig = nullptr;
        results.Reset();
        runtimeCarIds.fill(-1);
    }
};

ThirtyCarCupState g_cupState;

const CarInfo* GetCarInfoByModelId(int modelId) {
    CarInfo* cars = GetCarInfoTable();
    if (cars == nullptr || modelId < 0 || modelId >= GetTotalCarModelCount()) {
        return nullptr;
    }

    return &cars[modelId];
}

char* MutableText(const char* text) {
    return const_cast<char*>(text != nullptr ? text : "");
}

char* GetPlayerNameFromSettings() {
    RaceSettingsRuntime* settings = GetRaceSettings();
    return settings != nullptr ? settings->playerName : MutableText("Player");
}

TrackInfo* GetCupCompletionTrack(int trackIndex, int difficultyTier, bool cupDC) {
    if (!cupDC && trackIndex == 4) {
        return nullptr;
    }

    EnsureProgressLoaded(trackIndex);

    TrackInfo* track = GetTrackInfoByRuntimeIndex(trackIndex);
    if (track == nullptr || track->difficultyRating != difficultyTier) {
        return nullptr;
    }

    return track;
}

void BuildParticipantName(int modelId, char (&outName)[16]) {
    const CarInfo* car = GetCarInfoByModelId(modelId);
    if (car == nullptr || car->displayName[0] == '\0') {
        std::snprintf(outName, sizeof(outName), "Car %02d", modelId);
        return;
    }

    std::snprintf(outName, sizeof(outName), "%.*s", 15, car->displayName);
}

void ReturnToCupResultFrontend() {
    uint8_t* needsFrontendRefresh = GetFrontendCupResultFlag();
    if (needsFrontendRefresh != nullptr) {
        *needsFrontendRefresh = 1;
    }

    void** gameStateFunction = GetGameStateFunctionPtr();
    void* menuInitializeFrontend = GetMenuInitializeFrontend();
    if (gameStateFunction != nullptr) {
        *gameStateFunction = menuInitializeFrontend;
    }
}

} // anonymous namespace

bool IsThirtyCarCupActive() {
    return g_cupState.active && IsExtendedCupOpponentGrid(g_cupState.activeCup);
}

void ResetThirtyCarCupState() {
    g_cupState.Reset();
}

void StartThirtyCarCupState(
    int selectedCupIndex,
    CupProfile* cup,
    const RandomizedCup* cupConfig,
    const ExtendedCupResultsState& results
) {
    g_cupState.Reset();
    g_cupState.active = true;
    g_cupState.rosterGenerated = true;
    g_cupState.selectedCupIndex = selectedCupIndex;
    g_cupState.activeCup = cup;
    g_cupState.cupConfig = cupConfig;
    g_cupState.results = results;
}

void Hook_BuildGrid() {
    if (!IsThirtyCarCupActive()) {
        Orig_BuildGrid();
        return;
    }

    CupProfile* cup = g_cupState.activeCup;
    const int stageIndex = std::clamp(GetCurrentCupStageIndex(), 0, 15);
    const CupStage& stage = cup->stages[stageIndex];
    g_cupState.gridApplied = false;
    g_cupState.playerMovedToBack = false;
    g_cupState.runtimeCarIds.fill(-1);

    RaceSettingsRuntime* settings = GetRaceSettings();
    GameModeRuntime& gameMode = GetGameModeRuntime();
    PlayerRaceInfoRuntime* playerRaceInfo = GetPlayerRaceInfo();
    uint8_t* playerRaceInfo34Source = GetPlayerRaceInfo34Source();

    if (settings != nullptr) {
        settings->mode = MODE_CHAMPIONSHIP;
    }
    gameMode.mode = MODE_CHAMPIONSHIP;
    gameMode.trackId = stage.trackID;

    RVGL_ResetCurrentTrackSelectionState();

    if (playerRaceInfo != nullptr) {
        playerRaceInfo->mode = MODE_CHAMPIONSHIP;
        playerRaceInfo->laps = stage.numLaps;
        playerRaceInfo->mirror = stage.isMirror ? 1 : 0;
        playerRaceInfo->reverse = stage.isReverse ? 1 : 0;

        if (settings != nullptr) {
            const uint8_t pickupsEnabled = settings->pickupsEnabled;
            gameMode.pickupsEnabled = pickupsEnabled;
            playerRaceInfo->pickupsEnabled = pickupsEnabled;
        }

        playerRaceInfo->countdown = gameMode.countdown;
        playerRaceInfo->unknown34 =
            playerRaceInfo34Source != nullptr ? static_cast<int>(*playerRaceInfo34Source) : 0;

        TrackInfo* track = GetTrackInfoByRuntimeIndex(stage.trackID);
        if (track != nullptr) {
            std::snprintf(
                playerRaceInfo->trackFolder,
                sizeof(playerRaceInfo->trackFolder),
                "%.*s",
                15,
                track->folderName
            );
        }
    }

    gameMode.reverse = stage.isReverse ? 1 : 0;
    gameMode.mirror = stage.isMirror ? 1 : 0;
    gameMode.laps = stage.numLaps;

    const int count = std::clamp(cup->numCars, 0, kMaxCupCars);
    Orig_AddParticipantAndCount(
        kPlayerRaceCarState,
        0,
        g_cupState.results.participants[0].modelId,
        g_cupState.results.participants[0].skinId,
        1,
        1,
        GetPlayerNameFromSettings()
    );

    for (int i = 1; i < count; ++i) {
        char name[16] = {};
        BuildParticipantName(g_cupState.results.participants[i].modelId, name);
        Orig_AddParticipantAndCount(
            kCpuRaceCarState,
            i,
            g_cupState.results.participants[i].modelId,
            g_cupState.results.participants[i].skinId,
            2,
            i + 1,
            name
        );
    }

    Orig_AssignStartPositions();
}

void ApplyThirtyCarCupGrid() {
    if (!IsThirtyCarCupActive()) {
        return;
    }

    const int carCount = GetParticipantCount();
    const int targetCarCount = std::clamp(g_cupState.activeCup->numCars, 0, kMaxCupCars);
    if (carCount <= kNativeCupCars || carCount > targetCarCount) {
        return;
    }

    std::array<Vec3, randomizerMaxCarCount> gridPositions = {};
    if (!CalculateThirtyCarGridPositions(carCount, targetCarCount, gridPositions)) {
        return;
    }

    for (int gridIndex = 0; gridIndex < targetCarCount; ++gridIndex) {
        SetCarPos(gridIndex, gridPositions[gridIndex]);
        g_cupState.runtimeCarIds[gridIndex] = gridIndex;
    }

    g_cupState.gridApplied = true;
}

void MoveThirtyCarCupPlayerToBackAfterRacePositions() {
    if (!IsThirtyCarCupActive() ||
        !g_cupState.gridApplied ||
        g_cupState.playerMovedToBack ||
        g_cupState.activeCup == nullptr) {
        return;
    }

    const int targetCarCount = std::clamp(g_cupState.activeCup->numCars, 0, kMaxCupCars);
    if (MoveRuntimeCarsToBackAfterRacePositions(g_cupState.runtimeCarIds, targetCarCount)) {
        g_cupState.playerMovedToBack = true;
    }
}

void Hook_UpdateCupPostRaceProgress() {
    if (!IsThirtyCarCupActive()) {
        Orig_UpdateCupPostRaceProgress();
        return;
    }

    RecordExtendedCupStageResultsOnce(g_cupState.active, g_cupState.activeCup, g_cupState.results);
    MirrorExtendedCupTables(g_cupState.activeCup, g_cupState.results);

    const int postRaceStateBefore = GetCupPostRaceState();
    const NativePendingSnapshot nativePendingBefore =
        CaptureNativePendingSnapshot(g_cupState.activeCup);

    {
        NativeCupClamp clamp(g_cupState.activeCup);
        Orig_UpdateCupPostRaceProgress();
    }

    const int postRaceStateAfter = GetCupPostRaceState();
    bool pointsChanged = false;
    if (postRaceStateBefore == 4 && postRaceStateAfter != 4) {
        FlushAllPendingPoints(g_cupState.activeCup, g_cupState.results);
        pointsChanged = true;
    }
    else if (postRaceStateBefore == 4 &&
             postRaceStateAfter == 4 &&
             DidNativeFlushPendingPoints(g_cupState.activeCup, nativePendingBefore)) {
        FlushAllPendingPoints(g_cupState.activeCup, g_cupState.results);
        ResetPendingPointTimer(g_cupState.results);
        pointsChanged = true;
    }
    else {
        pointsChanged = AdvancePendingPointsOnTimer(g_cupState.activeCup, g_cupState.results);
    }

    if (pointsChanged) {
        SortExtendedCupStandings(g_cupState.activeCup, g_cupState.results);
    }
    UpdateExtendedCupResultFromStandings(g_cupState.active, g_cupState.activeCup, g_cupState.results);
    MirrorExtendedCupTables(g_cupState.activeCup, g_cupState.results);
}

void PrepareThirtyCarCupStageFinished() {
    if (!IsThirtyCarCupActive()) {
        return;
    }

    RecordExtendedCupStageResultsOnce(g_cupState.active, g_cupState.activeCup, g_cupState.results);
    FlushAllPendingPoints(g_cupState.activeCup, g_cupState.results);
    SortExtendedCupStandings(g_cupState.activeCup, g_cupState.results);
    UpdateExtendedCupResultFromStandings(g_cupState.active, g_cupState.activeCup, g_cupState.results);
    MirrorExtendedCupTables(g_cupState.activeCup, g_cupState.results);
}

bool HandleThirtyCarCupOnStageFinished() {
    if (!IsThirtyCarCupActive()) {
        return false;
    }

    PrepareThirtyCarCupStageFinished();

    RVGL_RaceTeardownAndSave();
    RVGL_LevelDestroyAndFree();

    const int selectedCupIndex = GetCurrentCupIndex();
    const int difficultyTier = g_cupState.activeCup->difficultyRating;
    if (GetCupStageDirection() == 2) {
        ++GetCurrentCupStageIndex();
    }
    else {
        --GetCupTriesLeft();
        if (GetCupTriesLeft() < 0) {
            GetCupStageDirection() = 0;
            UpdateExtendedCupFailedResult(g_cupState.activeCup, g_cupState.results);
            MirrorExtendedCupTables(g_cupState.activeCup, g_cupState.results);
            ReturnToCupResultFrontend();
            return true;
        }
    }

    if (g_cupState.activeCup->numStages != GetCurrentCupStageIndex()) {
        Hook_BuildGrid();
        RVGL_LoadNextRaceFromPlayerRaceInfo();
        return true;
    }

    CupResultRuntime& result = GetCupResultRuntime();
    result.completedFlag = 1;
    UpdateExtendedCupResultFromStandings(g_cupState.active, g_cupState.activeCup, g_cupState.results);
    MirrorExtendedCupTables(g_cupState.activeCup, g_cupState.results);

    if (result.playerOverallRank <= g_cupState.activeCup->overallRequiredPlace && selectedCupIndex < 5) {
        const bool cupDC = IsCupDCEnabled();
        bool allAlreadyCompleted = true;

        for (int trackIndex = 0; trackIndex < 14; ++trackIndex) {
            TrackInfo* track = GetCupCompletionTrack(trackIndex, difficultyTier, cupDC);
            if (track == nullptr) {
                continue;
            }

            if ((track->trackProgressFlags & TRACKPROGRESS_COMPLETED) == 0) {
                allAlreadyCompleted = false;
                break;
            }
        }

        for (int trackIndex = 0; trackIndex < 14; ++trackIndex) {
            TrackInfo* track = GetCupCompletionTrack(trackIndex, difficultyTier, cupDC);
            if (track == nullptr) {
                continue;
            }

            track->trackProgressFlags =
                static_cast<TrackProgressFlags>(track->trackProgressFlags | TRACKPROGRESS_COMPLETED);
        }

        if (!allAlreadyCompleted) {
            bool allTierTracksCompleted = true;
            for (int trackIndex = 0; trackIndex < 14; ++trackIndex) {
                TrackInfo* track = GetCupCompletionTrack(trackIndex, difficultyTier, cupDC);
                if (track == nullptr) {
                    continue;
                }

                if ((track->trackProgressFlags & TRACKPROGRESS_COMPLETED) == 0) {
                    allTierTracksCompleted = false;
                    break;
                }
            }

            if (allTierTracksCompleted) {
                GetTierUnlockTrigger() = 0;
            }
        }
    }

    ReturnToCupResultFrontend();
    return true;
}

void Hook_DrawCupStandingsTable() {
    if (!IsThirtyCarCupActive()) {
        Orig_DrawCupStandingsTable();
        return;
    }

    DrawExtendedCupStandingsTable(
        g_cupState.active,
        g_cupState.activeCup,
        g_cupState.results,
        GetPlayerNameFromSettings()
    );
}

} // namespace Randomizer
