#include "MenuMod.h"
#include "Addresses.h"
#include "RVGLMenuStructs.h"
#include "RandomizerState.h"
#include "RVGLFunctions.h"
#include "RVGLMemory.h"
#include <algorithm>
#include <cstring>

// ============================================================================
// MenuMod.cpp
//
// Hooks for modifying menu interaction
// ============================================================================

namespace Randomizer {

FnBuildMenu Orig_BuildStartRaceMenu = nullptr;
FnBuildMenu Orig_BuildOptionsMenu = nullptr;
FnHandleMenuAction Orig_HandleStartRaceMenuAction = nullptr;
FnHandleMenuAction Orig_HandleOptionsMenuAction = nullptr;

namespace {

constexpr int kMenuSlotStride = 0x140;
constexpr int kMenuItemPointerOffset = 2;
constexpr int kSelectedItemIndexOffset = 0x38;
constexpr int kMenuPanelTitleLabelOffset = 0x04;
constexpr int kMenuPanelBuildFunctionOffset = 0x20;
constexpr int kMenuPanelActionFunctionOffset = 0x28;
constexpr int kMenuPanelDescriptorSize = 0x80;

// Custom labels use native locale slots. LocaleStringOverride keeps the
// original pointer so a temporary override can be restored safely.
constexpr int kKnockoutLabelId = 0x17;
constexpr int kEliminationFrequencyLabelId = 0x1E;
constexpr int kEliminationsAtOnceLabelId = 0x1F;
constexpr int kKnockedOutCarStateLabelId = 0x20;
constexpr int kModOptionsLabelId = 0x21;
constexpr int kKnockoutLapCountLabelId = 0x22;
constexpr int kNativeSingleRaceLabelId = 0x15;

constexpr char kKnockoutLabel[] = "Knockout";
constexpr char kKnockoutLapCountLabel[] = "Knockout Lap Count";
constexpr char kKnockoutLapCountSingleRace[] = "Single Race";
constexpr char kKnockoutLapCountAutomatic[] = "Automatic";
constexpr char kEliminationFrequencyLabel[] = "Knockout Lap Frequency";
constexpr char kEliminationsAtOnceLabel[] = "Knockout Eliminations Per Lap";
constexpr char kKnockedOutCarStateLabel[] = "Knocked-out Car State";
constexpr char kKnockedOutCarStateSolid[] = "Solid";
constexpr char kKnockedOutCarStateGhost[] = "Ghost";
constexpr char kModOptionsLabel[] = "Mod Settings";

constexpr int kSingleRaceMode = MODE_SINGLE_RACE;
constexpr int kFrontendConfirmAction = 5;
constexpr int kFrontendCancelAction = 4;
constexpr int kRaceSettingsRandomFlagsOffset = 0xA4;
constexpr int kGameModeRandomFlagsOffset = 0x2E;
constexpr int kMenuValueXOffset = 0x114;
constexpr int kMenuValueYOffset = 0x118;
constexpr int kMenuValueWidthOffset = 0x134;
constexpr int kMenuValueMaxWidthOffset = 0x138;
constexpr float kMenuValueTextWidth = 8.0f;
constexpr float kMenuValueTextHeight = 12.0f;
constexpr float kMenuValueExtraX = 32.0f;
constexpr float kKnockoutLapCountValueWidth = 88.0f;
constexpr float kKnockoutFrequencyValueWidth = 110.0f;
constexpr uint32_t kMenuValueEnabledColor = 0xffebebeb;
constexpr uint32_t kMenuValueDisabledColor = 0xff464646;
constexpr uint32_t RVA_OPTIONS_GAME_SETTINGS_MENU_ITEM = 0x00265460;
constexpr uint32_t RVA_OPTIONS_GAME_SETTINGS_MENU_PANEL = 0x002643a0;

using FnHandleGenericMenuAction = bool(*)(int slotIndex, uint32_t action);

NumericMenuValue g_carCountMenuValue = {};
NumericMenuValue g_knockoutLapCountMenuValue = {};
NumericMenuValue g_eliminationFrequencyMenuValue = {};
NumericMenuValue g_eliminationsAtOnceMenuValue = {};
NumericMenuValue g_knockedOutCarStateMenuValue = {};
MenuItemDescriptor g_knockoutMenuItem = {};
MenuItemDescriptor g_knockoutLapCountMenuItem = {};
MenuItemDescriptor g_eliminationFrequencyMenuItem = {};
MenuItemDescriptor g_eliminationsAtOnceMenuItem = {};
MenuItemDescriptor g_knockedOutCarStateMenuItem = {};
MenuItemDescriptor g_modOptionsMenuItem = {};
alignas(8) uint8_t g_modOptionsMenuPanel[kMenuPanelDescriptorSize] = {};
bool g_knockoutMenuItemInitialized = false;
bool g_modOptionsMenuItemInitialized = false;

struct LocaleStringOverride {
    int labelId;
    const char* replacement;
    char* original = nullptr;
    char** capturedLocaleStrings = nullptr;
    char** overriddenLocaleStrings = nullptr;
    bool originalCaptured = false;
};

LocaleStringOverride g_knockoutLabelOverride{
    kKnockoutLabelId,
    kKnockoutLabel
};
LocaleStringOverride g_knockoutLapCountLabelOverride{
    kKnockoutLapCountLabelId,
    kKnockoutLapCountLabel
};
LocaleStringOverride g_eliminationFrequencyLabelOverride{
    kEliminationFrequencyLabelId,
    kEliminationFrequencyLabel
};
LocaleStringOverride g_eliminationsAtOnceLabelOverride{
    kEliminationsAtOnceLabelId,
    kEliminationsAtOnceLabel
};
LocaleStringOverride g_knockedOutCarStateLabelOverride{
    kKnockedOutCarStateLabelId,
    kKnockedOutCarStateLabel
};
LocaleStringOverride g_modOptionsLabelOverride{
    kModOptionsLabelId,
    kModOptionsLabel
};
LocaleStringOverride g_singleRaceLabelOverride{
    kNativeSingleRaceLabelId,
    kKnockoutLabel
};

void InitializeKnockoutOptionsMenuItems();

int ClampRandomizerCarCount(int carCount) {
    return std::clamp(carCount, randomizerMinCarCount, randomizerMaxCarCount);
}

int ClampEliminationFrequency(int frequency) {
    return std::clamp(frequency, 1, 10);
}

int ClampEliminationsAtOnce(int eliminations) {
    return std::clamp(eliminations, 1, randomizerMaxCarCount - 1);
}

int ClampKnockedOutGhostMode(int mode) {
    return mode != 0 ? 1 : 0;
}

int ClampKnockoutLapCountMode(int mode) {
    return mode != 0 ? 1 : 0;
}

char* MutableText(const char* text) {
    return const_cast<char*>(text != nullptr ? text : "");
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

bool SaveOriginalLocaleString(LocaleStringOverride& stringOverride) {
    char** localeStrings = GetLocaleStrings();
    if (localeStrings == nullptr) {
        return false;
    }

    if (!stringOverride.originalCaptured ||
        stringOverride.capturedLocaleStrings != localeStrings) {
        stringOverride.original = localeStrings[stringOverride.labelId];
        stringOverride.capturedLocaleStrings = localeStrings;
        stringOverride.originalCaptured = true;
    }

    return true;
}

bool OverrideLocaleString(LocaleStringOverride& stringOverride) {
    if (!SaveOriginalLocaleString(stringOverride)) {
        return false;
    }

    char** localeStrings = GetLocaleStrings();
    localeStrings[stringOverride.labelId] = MutableText(stringOverride.replacement);
    stringOverride.overriddenLocaleStrings = localeStrings;
    return true;
}

void RestoreLocaleString(LocaleStringOverride& stringOverride) {
    if (!stringOverride.originalCaptured ||
        stringOverride.overriddenLocaleStrings == nullptr) {
        return;
    }

    if (GetLocaleStrings() == stringOverride.overriddenLocaleStrings) {
        stringOverride.overriddenLocaleStrings[stringOverride.labelId] =
            stringOverride.original;
    }

    stringOverride.overriddenLocaleStrings = nullptr;
}

void PatchModOptionsMenuLocaleStrings() {
    OverrideLocaleString(g_knockoutLapCountLabelOverride);
    OverrideLocaleString(g_eliminationFrequencyLabelOverride);
    OverrideLocaleString(g_eliminationsAtOnceLabelOverride);
    OverrideLocaleString(g_knockedOutCarStateLabelOverride);
}

void RestoreModOptionsMenuLocaleStrings() {
    RestoreLocaleString(g_knockoutLapCountLabelOverride);
    RestoreLocaleString(g_eliminationFrequencyLabelOverride);
    RestoreLocaleString(g_eliminationsAtOnceLabelOverride);
    RestoreLocaleString(g_knockedOutCarStateLabelOverride);
}

void PatchStartRaceMenuLocaleStrings() {
    OverrideLocaleString(g_knockoutLabelOverride);
}

void RestoreStartRaceMenuLocaleStrings() {
    RestoreLocaleString(g_knockoutLabelOverride);
}

void PatchOptionsMenuLocaleStrings() {
    OverrideLocaleString(g_modOptionsLabelOverride);
}

void RestoreOptionsMenuLocaleStrings() {
    RestoreLocaleString(g_modOptionsLabelOverride);
}

void PatchSingleRaceLabelForKnockout() {
    OverrideLocaleString(g_singleRaceLabelOverride);
}

void RestoreSingleRaceLabel() {
    RestoreLocaleString(g_singleRaceLabelOverride);
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
    RestoreStartRaceMenuLocaleStrings();

    RandomizerContext& ctx = GetRandomizerContext();
    ctx.knockoutState.menuSelectionActive = false;
    ctx.knockoutState.modeActive = false;
}

void PrepareKnockoutStartRaceMenuState() {
    RestoreSingleRaceLabel();
    RestoreStartRaceMenuLocaleStrings();
    GetRandomizerContext().knockoutState.menuSelectionActive = false;
}

void SyncKnockoutOptionValues() {
    RandomizerContext& ctx = GetRandomizerContext();
    ctx.knockoutState.lapCountMode =
        ClampKnockoutLapCountMode(ctx.knockoutState.lapCountMode);
    ctx.knockoutState.eliminationFrequencyLaps =
        ClampEliminationFrequency(ctx.knockoutState.eliminationFrequencyLaps);
    ctx.knockoutState.eliminationsPerEvent =
        ClampEliminationsAtOnce(ctx.knockoutState.eliminationsPerEvent);
    ctx.knockoutState.knockedOutGhostMode =
        ClampKnockedOutGhostMode(ctx.knockoutState.knockedOutGhostMode);
}

bool IncrementKnockoutLapCount(int panelIndex) {
    const bool changed = RVGL_IncrementNumericMenuValue(panelIndex);
    SyncKnockoutOptionValues();
    return changed;
}

bool DecrementKnockoutLapCount(int panelIndex) {
    const bool changed = RVGL_DecrementNumericMenuValue(panelIndex);
    SyncKnockoutOptionValues();
    return changed;
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

bool IncrementKnockedOutCarState(int panelIndex) {
    const bool changed = RVGL_IncrementNumericMenuValue(panelIndex);
    SyncKnockoutOptionValues();
    return changed;
}

bool DecrementKnockedOutCarState(int panelIndex) {
    const bool changed = RVGL_DecrementNumericMenuValue(panelIndex);
    SyncKnockoutOptionValues();
    return changed;
}

void DrawTextMenuValue(int panelIndex, int itemIndex, const char* text) {
    uint8_t* slots = GetMenuSlotStorage();
    MenuItemDescriptor* descriptor = GetMenuItemDescriptor(panelIndex, itemIndex);
    if (slots == nullptr || descriptor == nullptr ||
        descriptor->valueBinding == nullptr || descriptor->valueBinding->value == nullptr) {
        return;
    }

    uint8_t* panel = slots + panelIndex * kMenuSlotStride;
    const float x =
        *reinterpret_cast<float*>(panel + kMenuValueXOffset) +
        *reinterpret_cast<float*>(panel + kMenuValueWidthOffset) +
        kMenuValueExtraX;
    const float y =
        static_cast<float>(itemIndex << 4) +
        *reinterpret_cast<float*>(panel + kMenuValueYOffset);
    const float maxWidth = *reinterpret_cast<float*>(panel + kMenuValueMaxWidthOffset);
    const uint32_t color =
        (descriptor->flags & 2) == 0 ? kMenuValueDisabledColor : kMenuValueEnabledColor;

    RVGL_DrawUIText(
        x,
        y,
        kMenuValueTextWidth,
        kMenuValueTextHeight,
        color,
        MutableText(text),
        maxWidth,
        0
    );
}

void DrawKnockoutLapCountMenuValue(int panelIndex, int itemIndex) {
    MenuItemDescriptor* descriptor = GetMenuItemDescriptor(panelIndex, itemIndex);
    if (descriptor == nullptr || descriptor->valueBinding == nullptr ||
        descriptor->valueBinding->value == nullptr) {
        return;
    }

    const char* text = *descriptor->valueBinding->value != 0
        ? kKnockoutLapCountAutomatic
        : kKnockoutLapCountSingleRace;
    DrawTextMenuValue(panelIndex, itemIndex, text);
}

void DrawKnockedOutCarStateMenuValue(int panelIndex, int itemIndex) {
    MenuItemDescriptor* descriptor = GetMenuItemDescriptor(panelIndex, itemIndex);
    if (descriptor == nullptr || descriptor->valueBinding == nullptr ||
        descriptor->valueBinding->value == nullptr) {
        return;
    }

    const char* text = *descriptor->valueBinding->value != 0
        ? kKnockedOutCarStateGhost
        : kKnockedOutCarStateSolid;
    DrawTextMenuValue(panelIndex, itemIndex, text);
}

void DrawEliminationFrequencyMenuValue(int panelIndex, int itemIndex) {
    MenuItemDescriptor* descriptor = GetMenuItemDescriptor(panelIndex, itemIndex);
    if (descriptor == nullptr || descriptor->valueBinding == nullptr ||
        descriptor->valueBinding->value == nullptr) {
        return;
    }

    const int frequency = *descriptor->valueBinding->value;
    char text[14] = "Every Lap";
    if (frequency > 1) {
        std::snprintf(text, sizeof(text), "Every %d Laps", frequency);
    }
    DrawTextMenuValue(panelIndex, itemIndex, text);
}

// Called every time the "Mod Options" menu is opened in the UI
void BuildModOptionsMenu(int slotIndex) {
    PatchModOptionsMenuLocaleStrings();
    InitializeKnockoutOptionsMenuItems();

    RVGL_RegisterMenuItemInActiveMenu(
        slotIndex,
        reinterpret_cast<int*>(&g_knockoutLapCountMenuItem)
    );
    RVGL_RegisterMenuItemInActiveMenu(
        slotIndex,
        reinterpret_cast<int*>(&g_eliminationFrequencyMenuItem)
    );
    RVGL_RegisterMenuItemInActiveMenu(
        slotIndex,
        reinterpret_cast<int*>(&g_eliminationsAtOnceMenuItem)
    );
    RVGL_RegisterMenuItemInActiveMenu(
        slotIndex,
        reinterpret_cast<int*>(&g_knockedOutCarStateMenuItem)
    );
}

// Called on every action on the "Mod Options" menu (navigation with up/down arrows, choose option, cancel)
bool HandleModOptionsMenuAction(int slotIndex, uint32_t action) {

    // This menu has no submenus, confirm action does nothing
    if (action == kFrontendConfirmAction) {
        return false;
    }

    const auto handleGenericMenuAction =
        reinterpret_cast<FnHandleGenericMenuAction>(AbsFromRva(RVA_HANDLE_GENERIC_MENU_ACTION));

    const bool result = handleGenericMenuAction(slotIndex, action);

    if (action == kFrontendCancelAction) {
        RestoreModOptionsMenuLocaleStrings();
    }

    return result;
}

void WriteModOptionsMenuPanelInt(int offset, int value) {
    *reinterpret_cast<int*>(g_modOptionsMenuPanel + offset) = value;
}

void WriteModOptionsMenuPanelPointer(int offset, void* value) {
    *reinterpret_cast<void**>(g_modOptionsMenuPanel + offset) = value;
}

// Runs the first time the "Options" menu is opened in the UI
// After that, g_modOptionsMenuItemInitialized is true and this function just returns early
void InitializeModOptionsMenuItem() {
    if (g_modOptionsMenuItemInitialized) {
        return;
    }

    const auto* gameSettingsMenuItem = reinterpret_cast<const MenuItemDescriptor*>(
        AbsFromRva(RVA_OPTIONS_GAME_SETTINGS_MENU_ITEM)
    );
    const void* gameSettingsMenuPanel = reinterpret_cast<const void*>(
        AbsFromRva(RVA_OPTIONS_GAME_SETTINGS_MENU_PANEL)
    );

    g_modOptionsMenuItem = *gameSettingsMenuItem;
    g_modOptionsMenuItem.labelId = kModOptionsLabelId;
    g_modOptionsMenuItem.valueBinding =
        reinterpret_cast<NumericMenuValue*>(g_modOptionsMenuPanel);

    std::memcpy(g_modOptionsMenuPanel, gameSettingsMenuPanel, sizeof(g_modOptionsMenuPanel));
    WriteModOptionsMenuPanelInt(kMenuPanelTitleLabelOffset, kModOptionsLabelId);
    WriteModOptionsMenuPanelPointer(
        kMenuPanelBuildFunctionOffset,
        reinterpret_cast<void*>(BuildModOptionsMenu)
    );
    WriteModOptionsMenuPanelPointer(
        kMenuPanelActionFunctionOffset,
        reinterpret_cast<void*>(HandleModOptionsMenuAction)
    );

    g_modOptionsMenuItemInitialized = true;
}

void InitializeKnockoutOptionsMenuItems() {
    RandomizerContext& ctx = GetRandomizerContext();
    SyncKnockoutOptionValues();

    g_knockoutLapCountMenuValue.value = &ctx.knockoutState.lapCountMode;
    g_knockoutLapCountMenuValue.minValue = 0;
    g_knockoutLapCountMenuValue.maxValue = 1;
    g_knockoutLapCountMenuValue.step = 1;
    g_knockoutLapCountMenuValue.useSlider = false;

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

    g_knockedOutCarStateMenuValue.value = &ctx.knockoutState.knockedOutGhostMode;
    g_knockedOutCarStateMenuValue.minValue = 0;
    g_knockedOutCarStateMenuValue.maxValue = 1;
    g_knockedOutCarStateMenuValue.step = 1;
    g_knockedOutCarStateMenuValue.useSlider = false;

    const auto* carCountMenuItem = reinterpret_cast<const MenuItemDescriptor*>(
        AbsFromRva(RVA_SETTINGS_NCARS_MENU_ITEM)
    );

    g_knockoutLapCountMenuItem = *carCountMenuItem;
    g_knockoutLapCountMenuItem.labelId = kKnockoutLapCountLabelId;
    g_knockoutLapCountMenuItem.valueWidth = kKnockoutLapCountValueWidth;
    g_knockoutLapCountMenuItem.valueBinding = &g_knockoutLapCountMenuValue;
    g_knockoutLapCountMenuItem.drawValue = DrawKnockoutLapCountMenuValue;
    g_knockoutLapCountMenuItem.decrementValue = DecrementKnockoutLapCount;
    g_knockoutLapCountMenuItem.incrementValue = IncrementKnockoutLapCount;

    g_eliminationFrequencyMenuItem = *carCountMenuItem;
    g_eliminationFrequencyMenuItem.labelId = kEliminationFrequencyLabelId;
    g_eliminationFrequencyMenuItem.valueWidth = kKnockoutFrequencyValueWidth;
    g_eliminationFrequencyMenuItem.valueBinding = &g_eliminationFrequencyMenuValue;
    g_eliminationFrequencyMenuItem.drawValue = DrawEliminationFrequencyMenuValue;
    g_eliminationFrequencyMenuItem.decrementValue = DecrementEliminationFrequency;
    g_eliminationFrequencyMenuItem.incrementValue = IncrementEliminationFrequency;

    g_eliminationsAtOnceMenuItem = *carCountMenuItem;
    g_eliminationsAtOnceMenuItem.labelId = kEliminationsAtOnceLabelId;
    g_eliminationsAtOnceMenuItem.valueBinding = &g_eliminationsAtOnceMenuValue;
    g_eliminationsAtOnceMenuItem.decrementValue = DecrementEliminationsAtOnce;
    g_eliminationsAtOnceMenuItem.incrementValue = IncrementEliminationsAtOnce;

    g_knockedOutCarStateMenuItem = *carCountMenuItem;
    g_knockedOutCarStateMenuItem.labelId = kKnockedOutCarStateLabelId;
    g_knockedOutCarStateMenuItem.valueBinding = &g_knockedOutCarStateMenuValue;
    g_knockedOutCarStateMenuItem.drawValue = DrawKnockedOutCarStateMenuValue;
    g_knockedOutCarStateMenuItem.decrementValue = DecrementKnockedOutCarState;
    g_knockedOutCarStateMenuItem.incrementValue = IncrementKnockedOutCarState;
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

// Called every time the "Start Race" menu is opened in the UI
void Hook_BuildStartRaceMenu(int slotIndex) {
    PrepareKnockoutStartRaceMenuState();
    Orig_BuildStartRaceMenu(slotIndex);

    PatchStartRaceMenuLocaleStrings();
    InitializeKnockoutMenuItemFromSingleRace(slotIndex);
    if (g_knockoutMenuItemInitialized) {
        RVGL_RegisterMenuItemInActiveMenu(
            slotIndex,
            reinterpret_cast<int*>(&g_knockoutMenuItem)
        );
    }
}

// Called every time the "Options" menu is opened in the UI
void Hook_BuildOptionsMenu(int slotIndex) {
    Orig_BuildOptionsMenu(slotIndex);

    PatchOptionsMenuLocaleStrings();
    InitializeModOptionsMenuItem();
    if (g_modOptionsMenuItemInitialized) {
        RVGL_RegisterMenuItemInActiveMenu(
            slotIndex,
            reinterpret_cast<int*>(&g_modOptionsMenuItem)
        );
    }
}

// Called on every action on the "Options" menu (navigation with up/down arrows,
// choose option, cancel).
bool Hook_HandleOptionsMenuAction(int slotIndex, uint32_t action) {
    const bool result = Orig_HandleOptionsMenuAction(slotIndex, action);
    if (action == kFrontendConfirmAction || action == kFrontendCancelAction) {
        RestoreOptionsMenuLocaleStrings();
    }

    return result;
}

// Called on every action on the "Start Race" menu (navigation with up/down arrows, choose option, cancel)
bool Hook_HandleStartRaceMenuAction(int slotIndex, uint32_t action) {
    const bool selectedKnockoutMenuItem =
        action == kFrontendConfirmAction &&
        GetSelectedMenuItemDescriptor(slotIndex) == &g_knockoutMenuItem;

    if (selectedKnockoutMenuItem) {
        SelectKnockoutMode();
    }
    else if (action == kFrontendConfirmAction && GetGameModeRuntime().mode == MODE_SELECT_FRONTEND) {
        ClearKnockoutMenuSelection();
    }

    const bool result = Orig_HandleStartRaceMenuAction(slotIndex, action);
    if (action == kFrontendConfirmAction || action == kFrontendCancelAction) {
        RestoreStartRaceMenuLocaleStrings();
    }

    return result;
}

} // namespace Randomizer
