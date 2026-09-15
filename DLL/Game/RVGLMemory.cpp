#include "RVGLMemory.h"
#include "Addresses.h"
#include "RVGLStructs.h"

namespace Randomizer {

RaceSettingsRuntime* GetRaceSettings() {
    return *reinterpret_cast<RaceSettingsRuntime**>(AbsFromRva(RVA_RACE_SETTINGS_PTR));
}

PlayerRaceInfoRuntime* GetPlayerRaceInfo() {
    return *reinterpret_cast<PlayerRaceInfoRuntime**>(AbsFromRva(RVA_PLAYER_RACE_INFO_PTR));
}

GameModeRuntime& GetGameModeRuntime() {
    return *reinterpret_cast<GameModeRuntime*>(AbsFromRva(RVA_GAME_MODE));
}

CupResultRuntime& GetCupResultRuntime() {
    return *reinterpret_cast<CupResultRuntime*>(AbsFromRva(RVA_CUP_RESULT));
}

char** GetLocaleStrings() {
    return *reinterpret_cast<char***>(AbsFromRva(RVA_LOCALE_STRINGS_PTR));
}

bool IsRaceFinished() {
    return *reinterpret_cast<bool*>(AbsFromRva(RVA_RACE_FINISHED_FLAG));
}

CarInfo* GetCarInfoTable() {
    return *reinterpret_cast<CarInfo**>(AbsFromRva(RVA_CAR_TABLE));
}

CarEntityRuntime* GetLiveCarById(int runtimeCarId) {
    CarEntityRuntime* car = *reinterpret_cast<CarEntityRuntime**>(AbsFromRva(RVA_CAR_LIST_HEAD));
    int visited = 0;

    while (car != nullptr && visited < 64) {
        if (car->nCarArrayIndex == runtimeCarId) {
            return car;
        }

        car = car->pNext;
        ++visited;
    }

    return nullptr;
}

CupProfile*& GetActiveCupRef() {
    return *reinterpret_cast<CupProfile**>(AbsFromRva(RVA_ACTIVE_CUP_PTR));
}

int& GetCurrentCupIndex() {
    return *reinterpret_cast<int*>(AbsFromRva(RVA_CURRENT_CUP_INDEX));
}

int& GetCurrentCupStageIndex() {
    return *reinterpret_cast<int*>(AbsFromRva(RVA_CURRENT_CUP_STAGE_INDEX));
}

int& GetCupTriesLeft() {
    return *reinterpret_cast<int*>(AbsFromRva(RVA_CUP_TRIES_LEFT));
}

int& GetCupStageDirection() {
    return *reinterpret_cast<int*>(AbsFromRva(RVA_CUP_STAGE_DIRECTION));
}

CupPostRaceState& GetCupPostRaceState() {
    return *reinterpret_cast<CupPostRaceState*>(AbsFromRva(RVA_CUP_POST_RACE_STATE));
}

int& GetNativeParticipantCount() {
    return *reinterpret_cast<int*>(AbsFromRva(RVA_RACE_PARTICIPANT_COUNT));
}

CupParticipantEntry* GetNativeCupParticipants() {
    return reinterpret_cast<CupParticipantEntry*>(AbsFromRva(RVA_NATIVE_CUP_PARTICIPANTS));
}

CupParticipantEntry* GetNativeCupStandings() {
    return reinterpret_cast<CupParticipantEntry*>(AbsFromRva(RVA_NATIVE_CUP_STANDINGS_SORTED));
}

bool IsCupDCEnabled() {
    return *reinterpret_cast<bool*>(AbsFromRva(RVA_CUP_DC));
}

bool IsRandomSkinEnabled() {
    return *reinterpret_cast<bool*>(AbsFromRva(RVA_RANDOM_SKINS_ENABLED));
}

int* GetPostRaceMenuDisplayState() {
    return *reinterpret_cast<int**>(AbsFromRva(RVA_POST_RACE_MENU_DISPLAY_STATE_PTR));
}

UiViewportRuntime* GetUiViewportRuntime() {
    UiViewportRuntime** viewportSlot =
        *reinterpret_cast<UiViewportRuntime***>(AbsFromRva(RVA_UI_VIEWPORT_PTR_PTR));
    return viewportSlot != nullptr ? *viewportSlot : nullptr;
}

float GetCupProgressUiCoordScale() {
    return *reinterpret_cast<float*>(AbsFromRva(RVA_CUP_PROGRESS_UI_COORD_SCALE));
}

uint8_t* GetCarModelHalfScaleFlagSource() {
    return *reinterpret_cast<uint8_t**>(AbsFromRva(RVA_DRINKME_ENABLED_PTR));
}

int& GetTierUnlockTrigger() {
    return *reinterpret_cast<int*>(AbsFromRva(RVA_TIER_UNLOCK_TRIGGER));
}

uint8_t* GetFrontendCupResultFlag() {
    return *reinterpret_cast<uint8_t**>(AbsFromRva(RVA_FRONTEND_CUP_RESULT_FLAG_PTR));
}

void** GetGameStateFunctionPtr() {
    return *reinterpret_cast<void***>(AbsFromRva(RVA_GAME_STATE_FUNCTION_PTR));
}

void* GetMenuInitializeFrontend() {
    return *reinterpret_cast<void**>(AbsFromRva(RVA_MENU_INITIALIZE_FRONTEND_PTR));
}

} // namespace Randomizer
