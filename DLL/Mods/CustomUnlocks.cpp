#include "CustomUnlocks.h"
#include "Addresses.h"
#include "RandomizerState.h"
#include "TrackHooks.h"
#include "CarHooks.h"
#include "Logger.h"

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

const CustomUnlockCondition* GetCarCustomUnlockCondition(int carIndex) {
    RandomizedCar* carConfig = GetCarConfigByRuntimeIndex(carIndex);
    if (carConfig == nullptr || !carConfig->customUnlock.has_value()) {
        return nullptr;
    }

    return &carConfig->customUnlock.value();
}

bool ShouldBypassCustomCarUnlocks() {
    const int unlockChecksEnabled = *reinterpret_cast<int*>(AbsFromRva(RVA_UNLOCK_CHECKS_ENABLED));
    const bool forceUnlockAll = *reinterpret_cast<bool*>(AbsFromRva(RVA_FORCE_UNLOCK_ALL_CARS));
    return unlockChecksEnabled == 0 || forceUnlockAll;
}

void LogMissingCustomCarUnlockOnce(int carIndex, const CarInfo& car) {
    static std::unordered_set<int> loggedCarIndices;
    if (!loggedCarIndices.insert(carIndex).second) {
        return;
    }

    Logger::TimestampLogf(
        "[UpdateCarSelectability] Warning: Custom unlock obtain %d for car %d ('%s') has no customUnlock config.",
        static_cast<int>(car.obtainCondition),
        carIndex,
        car.internalName
    );
}

void UpdateCarCustomUnlocks() {

    CarInfo* rawPool = GetCarPool();
    int carCount = GetRuntimeCarCount();

    if (rawPool == nullptr || carCount <= 0 || ShouldBypassCustomCarUnlocks()) {
        return;
    }

    for (int i = 0; i < carCount; ++i) {
        CarInfo& currentCar = rawPool[i];
        const int32_t obtain = static_cast<int32_t>(currentCar.obtainCondition);

        if (!IsCustomObtain(obtain)) {
            continue;
        }

        const CustomUnlockCondition* customUnlock = GetCarCustomUnlockCondition(i);
        if (customUnlock == nullptr) {
            currentCar.selectableByPlayer = false;
            LogMissingCustomCarUnlockOnce(i, currentCar);
            continue;
        }

        currentCar.selectableByPlayer = EvaluateCustomUnlock(
            UnlockTargetKind::Car,
            i,
            obtain,
            customUnlock
        );
    }
}

} // namespace Randomizer
