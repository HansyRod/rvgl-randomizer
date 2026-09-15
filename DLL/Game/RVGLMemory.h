#pragma once
#include "RVGLRaceStructs.h"
#include "RVGLStructs.h"
#include <cstdint>

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
    CupPostRaceState& GetCupPostRaceState();
    int& GetNativeParticipantCount();
    CupParticipantEntry* GetNativeCupParticipants();
    CupParticipantEntry* GetNativeCupStandings();
    bool IsCupDCEnabled();
    bool IsRandomSkinEnabled();
    int* GetPostRaceMenuDisplayState();
    UiViewportRuntime* GetUiViewportRuntime();
    float GetCupProgressUiCoordScale();
    uint8_t* GetCarModelHalfScaleFlagSource();
    int& GetTierUnlockTrigger();
    uint8_t* GetFrontendCupResultFlag();
    void** GetGameStateFunctionPtr();
    void* GetMenuInitializeFrontend();

} // namespace Randomizer
