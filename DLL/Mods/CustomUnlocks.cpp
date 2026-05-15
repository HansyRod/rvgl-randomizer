#include "CustomUnlocks.h"
#include "Addresses.h"
#include "RandomizerState.h"
#include "TrackHooks.h"

namespace Randomizer {

bool HasTrackProgressFlag(int trackIndex, TrackProgressFlags flag) {
    TrackInfo* track = GetTrackInfoByRuntimeIndex(trackIndex);
    return track != nullptr && (track->trackProgressFlags & flag) != 0;
}

int CountTracksWithProgressFlag(TrackProgressFlags flag) {
    const int trackCount = GetRuntimeTrackCount();
    int count = 0;

    for (int trackIndex = 0; trackIndex < trackCount; ++trackIndex) {
        if (HasTrackProgressFlag(trackIndex, flag)) {
            ++count;
        }
    }

    return count;
}

bool IsDefaultObtain(int32_t obtain) {
    return obtain >= -1 && obtain <= 5;
}

bool IsCustomObtain(int32_t obtain) {
    return obtain > 5;
}

bool HasRaceWin(int trackIndex) {
    return HasTrackProgressFlag(trackIndex, TRACKPROGRESS_RACE_WON);
}

bool HasPracticeStar(int trackIndex) {
    return HasTrackProgressFlag(trackIndex, TRACKPROGRESS_PRACTICE_STAR);
}

bool HasNormalTimeTrialBeaten(int trackIndex) {
    return HasTrackProgressFlag(trackIndex, TRACKPROGRESS_NORMAL_CHALLENGE_BEATEN);
}

int CountRaceWins() {
    return CountTracksWithProgressFlag(TRACKPROGRESS_RACE_WON);
}

int CountPracticeStars() {
    return CountTracksWithProgressFlag(TRACKPROGRESS_PRACTICE_STAR);
}

int CountNormalTimeTrialsBeaten() {
    return CountTracksWithProgressFlag(TRACKPROGRESS_NORMAL_CHALLENGE_BEATEN);
}

int GetStuntArenaStarsFound() {
    return *reinterpret_cast<int*>(AbsFromRva(RVA_TOTAL_STARS_EARNED));
}

void MarkArchipelagoItemReceived(const std::string& itemName) {
    if (itemName.empty()) {
        return;
    }

    ArchipelagoRuntimeState& archipelagoState = GetRandomizerContext().archipelagoState;
    std::lock_guard<std::mutex> lock(archipelagoState.itemMutex);
    archipelagoState.receivedItems.insert(itemName);
}

bool HasArchipelagoItem(const std::string& itemName) {
    if (itemName.empty()) {
        return false;
    }

    ArchipelagoRuntimeState& archipelagoState = GetRandomizerContext().archipelagoState;
    std::lock_guard<std::mutex> lock(archipelagoState.itemMutex);
    return archipelagoState.receivedItems.find(itemName) != archipelagoState.receivedItems.end();
}

void ClearArchipelagoItems() {
    ArchipelagoRuntimeState& archipelagoState = GetRandomizerContext().archipelagoState;
    std::lock_guard<std::mutex> lock(archipelagoState.itemMutex);
    archipelagoState.receivedItems.clear();
}

bool EvaluateCustomUnlock(
    UnlockTargetKind targetKind,
    int targetIndex,
    int32_t obtain,
    const CustomUnlockCondition* condition
) {
    (void)targetKind;
    (void)targetIndex;

    switch (static_cast<CustomUnlockMethod>(obtain)) {
    case CustomUnlockMethod::ArchipelagoItem:
        return condition != nullptr && HasArchipelagoItem(condition->archipelagoItem);
    default:
        // Other custom unlock methods are intentionally locked until their
        // progress/query implementations are wired into the evaluator.
        return false;
    }
}

} // namespace Randomizer
