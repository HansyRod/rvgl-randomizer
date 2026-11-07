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
    const MenuItemDescriptor* GetInRaceNextTrackMenuItem();
    MenuItemDescriptor* GetNativeResolutionItem();
    void* GetWindow();
    uint8_t& GetFullscreenFlag();
    uint8_t& GetNoresFlag();
    void* GetRenderMutex();
    uint8_t& GetRenderUpdatingFlag();
    PlayerRaceInfoRuntime* GetPlayerRaceInfo();
    int* GetRaceTransitionRequest();
    GameModeRuntime& GetGameModeRuntime();
    StuntArenaProgressRuntime* GetStuntArenaProgress();
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
