#pragma once

#include "RVGLRaceStructs.h"
#include <cstdint>

struct CarInfo;
struct CupResultRuntime;
struct CupProfile;
struct CupParticipantEntry;

namespace Randomizer {

RaceSettingsRuntime* GetRaceSettings();
PlayerRaceInfoRuntime* GetPlayerRaceInfo();
GameModeRuntime& GetGameModeRuntime();
CupResultRuntime& GetCupResultRuntime();
char** GetLocaleStrings();
bool IsRaceFinished();
CarInfo* GetCarInfoTable();
CarEntityRuntime* GetLiveCarById(int runtimeCarId);
CupProfile*& GetActiveCupRef();
int& GetCurrentCupIndex();
int& GetCurrentCupStageIndex();
int& GetCupTriesLeft();
int& GetCupStageDirection();
int& GetCupPostRaceState();
int& GetNativeParticipantCount();
CupParticipantEntry* GetNativeCupParticipants();
CupParticipantEntry* GetNativeCupStandings();
bool IsCupDCEnabled();
bool IsRandomCarColorEnabled();
int* GetPostRaceMenuDisplayState();
UiViewportRuntime* GetUiViewportRuntime();
float GetCupProgressUiCoordScale();
uint8_t* GetPlayerRaceInfo34Source();
int& GetTierUnlockTrigger();
uint8_t* GetFrontendCupResultFlag();
void** GetGameStateFunctionPtr();
void* GetMenuInitializeFrontend();

} // namespace Randomizer
