#pragma once
#include "RVGLRaceStructs.h"
#include "RVGLStructs.h"
#include "RVGLMenuStructs.h"
#include <cstdint>

namespace Randomizer {

    RaceSettingsRuntime* GetRaceSettings();
    int GetActivePlayerSlot();
    MenuPanelDescriptor* GetCarSelectionMenuPanel();
    MenuPanelSlot* GetMenuPanelSlot(int slotIndex);
    PlayerRaceInfoRuntime* GetPlayerRaceInfo();
    GameModeRuntime& GetGameModeRuntime();
    CupResultRuntime& GetCupResultRuntime();
    char** GetLocaleStrings();
    bool IsRaceFinished();
    bool IsLoadingScreenActive();
    CarInfo* GetCarInfoTable();
    CarEntityRuntime* GetLocalPlayerCar();
    RaceResultEntry* GetRaceResultTable();
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
