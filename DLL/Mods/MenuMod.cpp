#include "MenuMod.h"
#include "Addresses.h"
#include "RVGLMenuStructs.h"
#include "RandomizerState.h"
#include "RVGLFunctions.h"
#include "RVGLMemory.h"
#include <algorithm>

// ============================================================================
// MenuMod.cpp
//
// Hooks for modifying menu interaction
// ============================================================================

namespace Randomizer {

FnBuildMenu Orig_BuildStartRaceMenu = nullptr;
FnHandleMenuAction Orig_HandleStartRaceMenuAction = nullptr;
FnRegisterMenuItemInActiveMenu Orig_RegisterMenuItemInActiveMenu = nullptr;

namespace {

constexpr int kMenuSlotStride = 0x140;
constexpr int kMenuItemPointerOffset = 2;
constexpr int kSelectedItemIndexOffset = 0x38;

// Temporary custom labels. These patch native locale slots for now; the copied
// locale table approach needs more investigation before it is safe in menus.
constexpr int kKnockoutLabelId = 0x17;
constexpr int kEliminationFrequencyLabelId = 0x1E;
constexpr int kEliminationsAtOnceLabelId = 0x1F;
constexpr int kNativeSingleRaceLabelId = 0x15;

constexpr char kKnockoutLabel[] = "Knockout";
constexpr char kEliminationFrequencyLabel[] = "Knockout Lap Frequency";
constexpr char kEliminationsAtOnceLabel[] = "Knockout Eliminations Per Lap";

constexpr int kSingleRaceMode = MODE_SINGLE_RACE;
constexpr int kFrontendConfirmAction = 5;
constexpr int kRaceSettingsRandomFlagsOffset = 0xA4;
constexpr int kGameModeRandomFlagsOffset = 0x2E;

NumericMenuValue g_carCountMenuValue = {};
NumericMenuValue g_eliminationFrequencyMenuValue = {};
NumericMenuValue g_eliminationsAtOnceMenuValue = {};
MenuItemDescriptor g_knockoutMenuItem = {};
MenuItemDescriptor g_eliminationFrequencyMenuItem = {};
MenuItemDescriptor g_eliminationsAtOnceMenuItem = {};
bool g_knockoutMenuItemInitialized = false;
char* g_originalSingleRaceLabel = nullptr;
bool g_originalSingleRaceLabelCaptured = false;

int ClampRandomizerCarCount(int carCount) {
    return std::clamp(carCount, randomizerMinCarCount, randomizerMaxCarCount);
}

int ClampEliminationFrequency(int frequency) {
    return std::clamp(frequency, 1, 10);
}

int ClampEliminationsAtOnce(int eliminations) {
    return std::clamp(eliminations, 1, randomizerMaxCarCount - 1);
}

uint8_t* GetMenuSlotStorage() {
    return *reinterpret_cast<uint8_t**>(AbsFromRva(RVA_MENU_SLOTS_PTR));
}

MenuItemDescriptor* GetMenuItemDescriptor(int slotIndex, int itemIndex) {
    uint8_t* slots = GetMenuSlotStorage();
    if (slots == nullptr || itemIndex < 0) {
        return nullptr;
    }

    const int pointerIndex = itemIndex + kMenuItemPointerOffset + slotIndex * 0x28;
    return *reinterpret_cast<MenuItemDescriptor**>(slots + pointerIndex * sizeof(void*));
}

MenuItemDescriptor* GetSelectedMenuItemDescriptor(int slotIndex) {
    uint8_t* slots = GetMenuSlotStorage();
    if (slots == nullptr) {
        return nullptr;
    }

    uint8_t* activeMenu = *reinterpret_cast<uint8_t**>(slots + slotIndex * kMenuSlotStride);
    if (activeMenu == nullptr) {
        return nullptr;
    }

    const int selectedItemIndex =
        *reinterpret_cast<int*>(activeMenu + kSelectedItemIndexOffset);
    return GetMenuItemDescriptor(slotIndex, selectedItemIndex);
}

void PatchLocaleString(int labelId, const char* text) {
    char** localeStrings = GetLocaleStrings();
    if (localeStrings != nullptr) {
        localeStrings[labelId] = const_cast<char*>(text);
    }
}

void PatchKnockoutLocaleStrings() {
    PatchLocaleString(kKnockoutLabelId, kKnockoutLabel);
    PatchLocaleString(kEliminationFrequencyLabelId, kEliminationFrequencyLabel);
    PatchLocaleString(kEliminationsAtOnceLabelId, kEliminationsAtOnceLabel);
}

void PatchSingleRaceLabelForKnockout() {
    char** localeStrings = GetLocaleStrings();
    if (localeStrings == nullptr) {
        return;
    }

    if (!g_originalSingleRaceLabelCaptured) {
        g_originalSingleRaceLabel = localeStrings[kNativeSingleRaceLabelId];
        g_originalSingleRaceLabelCaptured = true;
    }

    localeStrings[kNativeSingleRaceLabelId] = const_cast<char*>(kKnockoutLabel);
}

void RestoreSingleRaceLabel() {
    char** localeStrings = GetLocaleStrings();
    if (localeStrings == nullptr || !g_originalSingleRaceLabelCaptured) {
        return;
    }

    localeStrings[kNativeSingleRaceLabelId] = g_originalSingleRaceLabel;
}

void InitializeKnockoutMenuItemFromSingleRace(int slotIndex) {
    if (g_knockoutMenuItemInitialized) {
        return;
    }

    MenuItemDescriptor* singleRaceMenuItem = GetMenuItemDescriptor(slotIndex, 0);
    if (singleRaceMenuItem == nullptr) {
        return;
    }

    g_knockoutMenuItem = *singleRaceMenuItem;
    g_knockoutMenuItem.labelId = kKnockoutLabelId;
    g_knockoutMenuItemInitialized = true;
}

void SelectKnockoutMode() {
    RandomizerContext& ctx = GetRandomizerContext();
    ctx.knockoutState.menuSelectionActive = true;
    ctx.knockoutState.modeActive = true;

    GameModeRuntime& gameMode = GetGameModeRuntime();
    gameMode.mode = kSingleRaceMode;

    RaceSettingsRuntime* settings = GetRaceSettings();
    const uint16_t randomFlags = settings != nullptr
        ? *reinterpret_cast<uint16_t*>(
            reinterpret_cast<uint8_t*>(settings) + kRaceSettingsRandomFlagsOffset
        )
        : 0;

    *reinterpret_cast<uint16_t*>(
        reinterpret_cast<uint8_t*>(&gameMode) + kGameModeRandomFlagsOffset
    ) = randomFlags;

    PatchSingleRaceLabelForKnockout();
}

void ClearKnockoutMenuSelection() {
    RestoreSingleRaceLabel();

    RandomizerContext& ctx = GetRandomizerContext();
    ctx.knockoutState.menuSelectionActive = false;
    ctx.knockoutState.modeActive = false;
}

void PrepareKnockoutStartRaceMenuState() {
    RestoreSingleRaceLabel();
    GetRandomizerContext().knockoutState.menuSelectionActive = false;
}

void SyncKnockoutOptionValues() {
    RandomizerContext& ctx = GetRandomizerContext();
    ctx.knockoutState.eliminationFrequencyLaps =
        ClampEliminationFrequency(ctx.knockoutState.eliminationFrequencyLaps);
    ctx.knockoutState.eliminationsPerEvent =
        ClampEliminationsAtOnce(ctx.knockoutState.eliminationsPerEvent);
}

bool IncrementEliminationFrequency(int panelIndex) {
    const bool changed = RVGL_IncrementNumericMenuValue(panelIndex);
    SyncKnockoutOptionValues();
    return changed;
}

bool DecrementEliminationFrequency(int panelIndex) {
    const bool changed = RVGL_DecrementNumericMenuValue(panelIndex);
    SyncKnockoutOptionValues();
    return changed;
}

bool IncrementEliminationsAtOnce(int panelIndex) {
    const bool changed = RVGL_IncrementNumericMenuValue(panelIndex);
    SyncKnockoutOptionValues();
    return changed;
}

bool DecrementEliminationsAtOnce(int panelIndex) {
    const bool changed = RVGL_DecrementNumericMenuValue(panelIndex);
    SyncKnockoutOptionValues();
    return changed;
}

void InitializeKnockoutOptionsMenuItems() {
    RandomizerContext& ctx = GetRandomizerContext();
    SyncKnockoutOptionValues();

    g_eliminationFrequencyMenuValue.value = &ctx.knockoutState.eliminationFrequencyLaps;
    g_eliminationFrequencyMenuValue.minValue = 1;
    g_eliminationFrequencyMenuValue.maxValue = 10;
    g_eliminationFrequencyMenuValue.step = 1;
    g_eliminationFrequencyMenuValue.useSlider = false;

    g_eliminationsAtOnceMenuValue.value = &ctx.knockoutState.eliminationsPerEvent;
    g_eliminationsAtOnceMenuValue.minValue = 1;
    g_eliminationsAtOnceMenuValue.maxValue = randomizerMaxCarCount - 1;
    g_eliminationsAtOnceMenuValue.step = 1;
    g_eliminationsAtOnceMenuValue.useSlider = false;

    const auto* carCountMenuItem = reinterpret_cast<const MenuItemDescriptor*>(
        AbsFromRva(RVA_SETTINGS_NCARS_MENU_ITEM)
    );

    g_eliminationFrequencyMenuItem = *carCountMenuItem;
    g_eliminationFrequencyMenuItem.labelId = kEliminationFrequencyLabelId;
    g_eliminationFrequencyMenuItem.valueBinding = &g_eliminationFrequencyMenuValue;
    g_eliminationFrequencyMenuItem.decrementValue = DecrementEliminationFrequency;
    g_eliminationFrequencyMenuItem.incrementValue = IncrementEliminationFrequency;

    g_eliminationsAtOnceMenuItem = *carCountMenuItem;
    g_eliminationsAtOnceMenuItem.labelId = kEliminationsAtOnceLabelId;
    g_eliminationsAtOnceMenuItem.valueBinding = &g_eliminationsAtOnceMenuValue;
    g_eliminationsAtOnceMenuItem.decrementValue = DecrementEliminationsAtOnce;
    g_eliminationsAtOnceMenuItem.incrementValue = IncrementEliminationsAtOnce;
}

bool IsFrontendCarCountSettingsDescriptor(int* descriptor) {
    return GetGameModeRuntime().mode == MODE_SELECT_FRONTEND &&
        descriptor == reinterpret_cast<int*>(AbsFromRva(RVA_SETTINGS_NCARS_MENU_ITEM));
}

} // anonymous namespace

bool IncrementRandomizerCarCount(int panelIndex) {
    bool changed = RVGL_IncrementNumericMenuValue(panelIndex);
    SyncCarCountToVanillaSettings();
    return changed;
}

bool DecrementRandomizerCarCount(int panelIndex) {
    bool changed = RVGL_DecrementNumericMenuValue(panelIndex);
    SyncCarCountToVanillaSettings();
    return changed;
}

void SyncCarCountToVanillaSettings() {

    RandomizerContext& ctx = GetRandomizerContext();
    int carsPerRace = ClampRandomizerCarCount(ctx.carState.carsPerRace);
    ctx.carState.carsPerRace = carsPerRace;

    int& nCars = *reinterpret_cast<int*>(AbsFromRva(RVA_NCARS));
    int& nCarsSetting = *reinterpret_cast<int*>(AbsFromRva(RVA_SETTINGS_NCARS));
    const int vanillaCarCount = carsPerRace < vanillaMaxCarCount
        ? carsPerRace
        : vanillaMaxCarCount;

    nCars = vanillaCarCount;
    nCarsSetting = vanillaCarCount;
}

void PatchCarCountMenuDescriptor() {
    RandomizerContext& ctx = GetRandomizerContext();
    ctx.carState.carsPerRace = ClampRandomizerCarCount(ctx.carState.carsPerRace);

    g_carCountMenuValue.value = &ctx.carState.carsPerRace;
    g_carCountMenuValue.minValue = randomizerMinCarCount;
    g_carCountMenuValue.maxValue = randomizerMaxCarCount;
    g_carCountMenuValue.step = 1;
    g_carCountMenuValue.useSlider = false;

    auto* carCountMenuItem = reinterpret_cast<MenuItemDescriptor*>(
        AbsFromRva(RVA_SETTINGS_NCARS_MENU_ITEM)
    );

    carCountMenuItem->valueBinding = &g_carCountMenuValue;
    carCountMenuItem->drawValue = RVGL_DrawNumericMenuValue;
    carCountMenuItem->decrementValue = DecrementRandomizerCarCount;
    carCountMenuItem->incrementValue = IncrementRandomizerCarCount;

    SyncCarCountToVanillaSettings();
}

void Hook_BuildStartRaceMenu(int slotIndex) {
    PrepareKnockoutStartRaceMenuState();
    Orig_BuildStartRaceMenu(slotIndex);

    PatchKnockoutLocaleStrings();
    InitializeKnockoutMenuItemFromSingleRace(slotIndex);
    if (g_knockoutMenuItemInitialized) {
        RVGL_RegisterMenuItemInActiveMenu(
            slotIndex,
            reinterpret_cast<int*>(&g_knockoutMenuItem)
        );
    }
}

bool Hook_HandleStartRaceMenuAction(int slotIndex, uint32_t action) {
    if (action == kFrontendConfirmAction &&
        GetSelectedMenuItemDescriptor(slotIndex) == &g_knockoutMenuItem) {
        SelectKnockoutMode();
    }
    else if (action == kFrontendConfirmAction && GetGameModeRuntime().mode == MODE_SELECT_FRONTEND) {
        ClearKnockoutMenuSelection();
    }

    return Orig_HandleStartRaceMenuAction(slotIndex, action);
}

void Hook_RegisterMenuItemInActiveMenu(int slotIndex, int* descriptor) {
    Orig_RegisterMenuItemInActiveMenu(slotIndex, descriptor);

    if (!IsFrontendCarCountSettingsDescriptor(descriptor)) {
        return;
    }

    PatchKnockoutLocaleStrings();
    InitializeKnockoutOptionsMenuItems();
    Orig_RegisterMenuItemInActiveMenu(
        slotIndex,
        reinterpret_cast<int*>(&g_eliminationFrequencyMenuItem)
    );
    Orig_RegisterMenuItemInActiveMenu(
        slotIndex,
        reinterpret_cast<int*>(&g_eliminationsAtOnceMenuItem)
    );
}

} // namespace Randomizer
