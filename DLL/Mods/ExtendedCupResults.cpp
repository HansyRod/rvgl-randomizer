#include "ExtendedCupResults.h"
#include "Logger.h"
#include "RVGLMemory.h"
#include <algorithm>
#include <cmath>

namespace Randomizer {

namespace {

int GetPointsForPosition(
    CupProfile* activeCup,
    const ExtendedCupResultsState& results,
    int zeroBasedPosition
) {
    if (zeroBasedPosition >= 0 && zeroBasedPosition < static_cast<int>(results.pointsTable.size())) {
        return results.pointsTable[zeroBasedPosition];
    }

    if (activeCup != nullptr &&
        zeroBasedPosition >= 0 &&
        zeroBasedPosition < vanillaMaxCarCount) {
        return activeCup->pointsTable[zeroBasedPosition];
    }

    return 0;
}

void AdvancePendingPointsOneStep(CupProfile* activeCup, ExtendedCupResultsState& results) {
    if (activeCup == nullptr) {
        return;
    }

    const int count = std::clamp(activeCup->numCars, 0, randomizerMaxCarCount);
    for (int i = 0; i < count; ++i) {
        CupParticipantEntry& participant = results.participants[i];
        if (participant.pendingPoints == 0) {
            continue;
        }

        const int step = participant.pendingPoints > 0 ? 1 : -1;
        participant.totalPoints += step;
        participant.pendingPoints -= step;
    }
}

bool HasPendingPoints(CupProfile* activeCup, const ExtendedCupResultsState& results) {
    if (activeCup == nullptr) {
        return false;
    }

    const int count = std::clamp(activeCup->numCars, 0, randomizerMaxCarCount);
    return std::any_of(
        results.participants.begin(),
        results.participants.begin() + count,
        [](const CupParticipantEntry& participant) {
            return participant.pendingPoints != 0;
        }
    );
}

} // anonymous namespace

void ExtendedCupResultsState::Reset() {
    lastRecordedStage = -1;
    pendingPointTimerInitialized = false;
    pendingPointNextTick = {};
    pointsTable.clear();
    participants = {};
    standings = {};
}

NativeCupClamp::NativeCupClamp(CupProfile* cup)
    : cup_(cup),
      savedNumCars_(cup != nullptr ? cup->numCars : 0),
      savedParticipantCount_(GetNativeParticipantCount()) {
    if (cup_ != nullptr && cup_->numCars > vanillaMaxCarCount) {
        cup_->numCars = vanillaMaxCarCount;
    }
    if (GetNativeParticipantCount() > vanillaMaxCarCount) {
        GetNativeParticipantCount() = vanillaMaxCarCount;
    }
}

NativeCupClamp::~NativeCupClamp() {
    if (cup_ != nullptr) {
        cup_->numCars = savedNumCars_;
    }
    GetNativeParticipantCount() = savedParticipantCount_;
}

void MirrorExtendedCupTables(CupProfile* activeCup, const ExtendedCupResultsState& results) {
    const int count = (std::min)(activeCup != nullptr ? activeCup->numCars : 0, vanillaMaxCarCount);
    CupParticipantEntry* nativeParticipants = GetNativeCupParticipants();
    CupParticipantEntry* nativeStandings = GetNativeCupStandings();

    for (int i = 0; i < count; ++i) {
        nativeParticipants[i] = results.participants[i];
        nativeStandings[i] = results.standings[i];
    }
}

void SortExtendedCupStandings(CupProfile* activeCup, ExtendedCupResultsState& results) {
    if (activeCup == nullptr) {
        return;
    }

    const int count = std::clamp(activeCup->numCars, 0, randomizerMaxCarCount);
    std::copy(
        results.participants.begin(),
        results.participants.begin() + count,
        results.standings.begin()
    );

    const int stage = std::clamp(GetCurrentCupStageIndex(), 0, 15);
    std::stable_sort(
        results.standings.begin(),
        results.standings.begin() + count,
        [stage](const CupParticipantEntry& lhs, const CupParticipantEntry& rhs) {
            if (lhs.totalPoints != rhs.totalPoints) {
                return lhs.totalPoints > rhs.totalPoints;
            }

            return lhs.finishPositionByStage[stage] < rhs.finishPositionByStage[stage];
        }
    );
}

void UpdateExtendedCupResultFromStandings(
    bool active,
    CupProfile* activeCup,
    const ExtendedCupResultsState& results
) {
    if (!active || activeCup == nullptr) {
        return;
    }

    const int count = std::clamp(activeCup->numCars, 0, randomizerMaxCarCount);
    CupResultRuntime& result = GetCupResultRuntime();

    result.playerOverallRank = count;
    for (int i = 0; i < count; ++i) {
        if (results.standings[i].participantIndex == 0) {
            result.playerOverallRank = i + 1;
            break;
        }
    }

    for (int i = 0; i < 3 && i < count; ++i) {
        result.standingsSnapshot[i] = results.standings[i].modelId;
    }
    result.playerFinalRank = results.participants[0].modelId;
}

void UpdateExtendedCupFailedResult(CupProfile* activeCup, const ExtendedCupResultsState& results) {
    if (activeCup == nullptr) {
        return;
    }

    const int count = std::clamp(activeCup->numCars, 0, randomizerMaxCarCount);
    CupResultRuntime& result = GetCupResultRuntime();

    result.completedFlag = 0;
    result.playerOverallRank = count;
    result.playerFinalRank = results.participants[0].modelId;

    for (int i = 0; i < 3; ++i) {
        const int participantIndex = i + 1;
        result.standingsSnapshot[i] =
            participantIndex < count ? results.participants[participantIndex].modelId : 0;
    }

    const int stage = GetCurrentCupStageIndex();
    if (stage < 0 || stage >= 16 || results.lastRecordedStage != stage) {
        return;
    }

    for (int participantIndex = 0; participantIndex < count; ++participantIndex) {
        const CupParticipantEntry& participant = results.participants[participantIndex];
        const int finishPosition = participant.finishPositionByStage[stage];
        if (finishPosition >= 0 && finishPosition < 3) {
            result.standingsSnapshot[finishPosition] = participant.modelId;
        }
    }

    const int playerFinishPosition = results.participants[0].finishPositionByStage[stage];
    result.playerOverallRank = std::clamp(playerFinishPosition + 1, 1, count);
}

void RecordExtendedCupStageResultsOnce(
    bool active,
    CupProfile* activeCup,
    ExtendedCupResultsState& results
) {
    if (!active || activeCup == nullptr) {
        return;
    }

    const bool raceFinished = IsRaceFinished();
    if (!raceFinished) {
        return;
    }

    const int stage = GetCurrentCupStageIndex();
    if (stage < 0 || stage >= 16 || results.lastRecordedStage == stage) {
        return;
    }

    const int count = std::clamp(activeCup->numCars, 0, randomizerMaxCarCount);
    for (int participantIndex = 0; participantIndex < count; ++participantIndex) {
        CarEntityRuntime* car = GetLiveCarById(participantIndex);

        int finishPosition = participantIndex;
        int finishTime = 0;
        if (car != nullptr) {
            finishPosition = car->finishPosition >= 0 ? car->finishPosition : car->racePositionIndex;
            finishTime = car->finishTimeMs;
        }

        finishPosition = std::clamp(finishPosition, 0, count - 1);
        CupParticipantEntry& participant = results.participants[participantIndex];
        participant.finishPositionByStage[stage] = finishPosition;
        participant.finishTimeByStage[stage] = finishTime;
        participant.pendingPoints = GetPointsForPosition(activeCup, results, finishPosition);
    }

    results.lastRecordedStage = stage;
    SortExtendedCupStandings(activeCup, results);
    UpdateExtendedCupResultFromStandings(active, activeCup, results);
    MirrorExtendedCupTables(activeCup, results);

    Logger::TimestampLogf(
        "[ThirtyCarCupMod] Recorded stage %d results for %d-car cup",
        stage,
        count
    );
}

NativePendingSnapshot CaptureNativePendingSnapshot(CupProfile* activeCup) {
    NativePendingSnapshot snapshot;
    CupParticipantEntry* nativeParticipants = GetNativeCupParticipants();
    const int count = std::clamp(activeCup != nullptr ? activeCup->numCars : 0, 0, vanillaMaxCarCount);

    for (int i = 0; i < count; ++i) {
        snapshot.pendingPoints[i] = nativeParticipants[i].pendingPoints;
    }

    return snapshot;
}

bool DidNativeFlushPendingPoints(CupProfile* activeCup, const NativePendingSnapshot& before) {
    CupParticipantEntry* nativeParticipants = GetNativeCupParticipants();
    const int count = std::clamp(activeCup != nullptr ? activeCup->numCars : 0, 0, vanillaMaxCarCount);
    bool hadPending = false;
    bool flushedMoreThanOnePoint = false;

    for (int i = 0; i < count; ++i) {
        const int pendingBefore = before.pendingPoints[i];
        if (pendingBefore == 0) {
            continue;
        }

        hadPending = true;
        if (nativeParticipants[i].pendingPoints != 0) {
            return false;
        }
        if (std::abs(pendingBefore) > 1) {
            flushedMoreThanOnePoint = true;
        }
    }

    return hadPending && flushedMoreThanOnePoint;
}

void ResetPendingPointTimer(ExtendedCupResultsState& results) {
    results.pendingPointTimerInitialized = false;
    results.pendingPointNextTick = {};
}

bool AdvancePendingPointsOnTimer(CupProfile* activeCup, ExtendedCupResultsState& results) {
    constexpr auto pendingPointInitialDelay = std::chrono::milliseconds(2000);
    constexpr auto pendingPointTick = std::chrono::milliseconds(200);

    if (GetCupPostRaceState() != 4 || !HasPendingPoints(activeCup, results)) {
        ResetPendingPointTimer(results);
        return false;
    }

    const auto now = std::chrono::steady_clock::now();
    if (!results.pendingPointTimerInitialized) {
        results.pendingPointTimerInitialized = true;
        results.pendingPointNextTick = now + pendingPointInitialDelay;
        return false;
    }

    bool advanced = false;
    while (now >= results.pendingPointNextTick && HasPendingPoints(activeCup, results)) {
        AdvancePendingPointsOneStep(activeCup, results);
        results.pendingPointNextTick += pendingPointTick;
        advanced = true;
    }

    return advanced;
}

void FlushAllPendingPoints(CupProfile* activeCup, ExtendedCupResultsState& results) {
    if (activeCup == nullptr) {
        return;
    }

    const int count = std::clamp(activeCup->numCars, 0, randomizerMaxCarCount);
    for (int i = 0; i < count; ++i) {
        CupParticipantEntry& participant = results.participants[i];
        participant.totalPoints += participant.pendingPoints;
        participant.pendingPoints = 0;
    }
}

} // namespace Randomizer
