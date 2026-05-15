#pragma once

#include <cstdint>
#include <string>
#include "RVGLStructs.h"

namespace Randomizer {

enum class UnlockTargetKind {
    Car,
    Track,
    Cup
};

enum class CustomUnlockMethod : int32_t {
    SpecificRaceWin = 6,
    SpecificPracticeStar = 7,
    SpecificTimeTrial = 8,
    RaceWinCount = 9,
    PracticeStarCount = 10,
    TimeTrialCount = 11,
    StuntArenaStarCount = 12,
    ArchipelagoItem = 13
};

struct CustomUnlockCondition {
    std::string trackFolder;
    int requiredCount = 0;
    std::string archipelagoItem;
};

bool HasTrackProgressFlag(int trackIndex, TrackProgressFlags flag);
int CountTracksWithProgressFlag(TrackProgressFlags flag);

bool IsDefaultObtain(int32_t obtain);
bool IsCustomObtain(int32_t obtain);

bool HasRaceWin(int trackIndex);
bool HasPracticeStar(int trackIndex);
bool HasNormalTimeTrialBeaten(int trackIndex);

int CountRaceWins();
int CountPracticeStars();
int CountNormalTimeTrialsBeaten();

int GetStuntArenaStarsFound();

void MarkArchipelagoItemReceived(const std::string& itemName);
bool HasArchipelagoItem(const std::string& itemName);
void ClearArchipelagoItems();

bool EvaluateCustomUnlock(
    UnlockTargetKind targetKind,
    int targetIndex,
    int32_t obtain,
    const CustomUnlockCondition* condition
);

const CustomUnlockCondition* GetCarCustomUnlockCondition(int carIndex);
bool ShouldBypassCustomCarUnlocks();
void LogMissingCustomCarUnlockOnce(int carIndex, const CarInfo& car);
void UpdateCarCustomUnlocks();

} // namespace Randomizer
