#pragma once

#include "RandomizerState.h"
#include <array>
#include <chrono>
#include <vector>

namespace Randomizer {

struct NativePendingSnapshot {
    std::array<int, vanillaMaxCarCount> pendingPoints = {};
};

struct ExtendedCupResultsState {
    int lastRecordedStage = -1;
    bool pendingPointTimerInitialized = false;
    std::chrono::steady_clock::time_point pendingPointNextTick = {};
    std::array<CupParticipantEntry, randomizerMaxCarCount> participants = {};
    std::array<CupParticipantEntry, randomizerMaxCarCount> standings = {};
    std::vector<int> pointsTable;

    void Reset();
};

class NativeCupClamp {
public:
    explicit NativeCupClamp(CupProfile* cup);
    ~NativeCupClamp();

private:
    CupProfile* cup_;
    int savedNumCars_;
    int savedParticipantCount_;
};

void MirrorExtendedCupTables(CupProfile* activeCup, const ExtendedCupResultsState& results);
void SortExtendedCupStandings(CupProfile* activeCup, ExtendedCupResultsState& results);
void UpdateExtendedCupResultFromStandings(
    bool active,
    CupProfile* activeCup,
    const ExtendedCupResultsState& results
);
void UpdateExtendedCupFailedResult(CupProfile* activeCup, const ExtendedCupResultsState& results);
void RecordExtendedCupStageResultsOnce(
    bool active,
    CupProfile* activeCup,
    ExtendedCupResultsState& results
);
NativePendingSnapshot CaptureNativePendingSnapshot(CupProfile* activeCup);
bool DidNativeFlushPendingPoints(CupProfile* activeCup, const NativePendingSnapshot& before);
void ResetPendingPointTimer(ExtendedCupResultsState& results);
bool AdvancePendingPointsOnTimer(CupProfile* activeCup, ExtendedCupResultsState& results);
void FlushAllPendingPoints(CupProfile* activeCup, ExtendedCupResultsState& results);

} // namespace Randomizer
