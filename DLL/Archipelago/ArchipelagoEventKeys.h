#pragma once

#include <string>

namespace Randomizer {

namespace ArchipelagoEventKeys {

constexpr const char* SingleRaceWin = "single_race_win";
constexpr const char* PracticeStar = "practice_star";
constexpr const char* TimeTrialNormal = "time_trial_normal";
constexpr const char* TimeTrialReverse = "time_trial_reverse";
constexpr const char* TimeTrialMirror = "time_trial_mirror";
constexpr const char* ChampionshipWin = "championship_win";
constexpr const char* StuntStar = "stunt_star";

int GetStuntStarCheckNumber(int starId);
std::string MakeIndexedEventKey(const char* eventName, int checkNumber);
std::string MakeNamedEventKey(const char* eventName, const char* name);

} // namespace ArchipelagoEventKeys

} // namespace Randomizer
