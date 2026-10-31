#include "MenuMod.h"
#include "CarHooks.h"
#include "Logger.h"
#include "LocaleUtils.h"
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
FnBuildMenu Orig_DrawPreRaceSummary = nullptr;
FnBuildMenu Orig_BuildInRaceMenu = nullptr;
FnHandleMenuAction Orig_HandleStartRaceMenuAction = nullptr;
FnHandleMenuAction Orig_HandleOptionsMenuAction = nullptr;
FnHandleMenuAction Orig_HandleInRaceMenuAction = nullptr;

namespace {

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
constexpr int kNextCarLabelId = 0x28;

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
constexpr float kMenuValueTextWidth = 8.0f;
constexpr float kMenuValueTextHeight = 12.0f;
constexpr float kMenuValueExtraX = 32.0f;
constexpr float kKnockoutLapCountValueWidth = 88.0f;
constexpr float kKnockoutFrequencyValueWidth = 110.0f;
constexpr uint32_t kMenuValueEnabledColor = 0xffebebeb;
constexpr uint32_t kMenuValueDisabledColor = 0xff464646;
constexpr uint32_t RVA_OPTIONS_GAME_SETTINGS_MENU_ITEM = 0x00265460;
constexpr uint32_t RVA_OPTIONS_GAME_SETTINGS_MENU_PANEL = 0x002643a0;

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
int g_nextCarModelId = -1;
int g_pendingInRaceCarModelId = -1;
NumericMenuValue g_nextCarMenuValue = {};
MenuItemDescriptor g_nextCarMenuItem = {};
LocaleStringOverride g_nextCarLabelOverride{kNextCarLabelId, "Next Car"};

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

MenuItemDescriptor* GetMenuItemDescriptor(int slotIndex, int itemIndex) {
    MenuPanelSlot* slot = GetMenuPanelSlot(slotIndex);
    if (slot == nullptr || itemIndex < 0 || itemIndex >= slot->itemCount || itemIndex >= 30) {
        return nullptr;
    }

    return slot->items[itemIndex];
}

MenuItemDescriptor* GetSelectedMenuItemDescriptor(int slotIndex) {
    MenuPanelSlot* slot = GetMenuPanelSlot(slotIndex);
    if (slot == nullptr || slot->panel == nullptr) {
        return nullptr;
    }

    const int selectedItemIndex = slot->panel->selectedItem;
    return GetMenuItemDescriptor(slotIndex, selectedItemIndex);
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
    if (!IsKnockoutModeEnabled()) {
        return;
    }

    RandomizerContext& ctx = GetRandomizerContext();
    ctx.knockoutState.menuSelectionActive = true;
    ctx.knockoutState.modeActive = true;

    GameModeRuntime& gameMode = GetGameModeRuntime();
    gameMode.mode = kSingleRaceMode;

    RaceSettingsRuntime* settings = GetRaceSettings();
    gameMode.randomCars = settings != nullptr ? settings->randomCars : 0;
    gameMode.randomTracks = settings != nullptr ? settings->randomTracks : 0;

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
    MenuPanelSlot* panel = GetMenuPanelSlot(panelIndex);
    MenuItemDescriptor* descriptor = GetMenuItemDescriptor(panelIndex, itemIndex);
    if (panel == nullptr || descriptor == nullptr ||
        descriptor->valueBinding == nullptr || descriptor->valueBinding->value == nullptr) {
        return;
    }

    const float x =
        panel->textX +
        panel->labelWidth +
        kMenuValueExtraX;
    const float y =
        static_cast<float>(itemIndex << 4) +
        panel->textY;
    const float maxWidth = panel->valueWidth;
    const uint32_t color =
        descriptor->flags.enabled ? kMenuValueEnabledColor : kMenuValueDisabledColor;

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

bool CanSwitchCarsInRace() {
    const int mode = GetGameModeRuntime().mode;
    return (mode == MODE_SINGLE_RACE || mode == MODE_PRACTICE ||
        mode == MODE_STUNT_ARENA || mode == MODE_TIME_TRIAL) &&
        GetLocalPlayerCar() != nullptr;
}

bool IsSelectableRaceCar(int modelId) {
    const CarInfo* cars = GetCarPool();
    return cars != nullptr && modelId >= 0 && modelId < GetRuntimeCarCount() &&
        cars[modelId].selectableByPlayer && !cars[modelId].isInvalid;
}

int FindNextSelectableRaceCar(int modelId, int direction) {
    const int carCount = GetRuntimeCarCount();
    if (carCount <= 0 || GetCarPool() == nullptr) {
        return -1;
    }
    int candidate = modelId >= 0 && modelId < carCount ? modelId : 0;
    for (int checked = 0; checked < carCount; ++checked) {
        candidate += direction;
        if (candidate >= carCount) {
            candidate = 0;
        } else if (candidate < 0) {
            candidate = carCount - 1;
        }
        if (IsSelectableRaceCar(candidate)) {
            return candidate;
        }
    }
    return -1;
}

bool CycleNextCar(int direction) {
    const int nextCar = FindNextSelectableRaceCar(g_nextCarModelId, direction);
    if (nextCar < 0 || nextCar == g_nextCarModelId) {
        return false;
    }
    g_nextCarModelId = nextCar;
    return true;
}

bool DecrementNextCar(int panelIndex) {
    return CycleNextCar(-1);
}

bool IncrementNextCar(int panelIndex) {
    return CycleNextCar(1);
}

void DrawNextCar(int panelIndex, int itemIndex) {
    if (!IsSelectableRaceCar(g_nextCarModelId)) {
        return;
    }
    const CarInfo& car = GetCarPool()[g_nextCarModelId];
    char name[sizeof(car.displayName) + 1] = {};
    std::memcpy(name, car.displayName, sizeof(car.displayName));
    DrawTextMenuValue(panelIndex, itemIndex, name);
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
    if (!IsKnockoutModeEnabled()) {
        return;
    }

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

    const bool result = RVGL_HandleGenericMenuAction(slotIndex, action);

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
    if (!IsThirtyCarModeEnabled()) {
        return false;
    }

    bool changed = RVGL_IncrementNumericMenuValue(panelIndex);
    SyncCarCountToVanillaSettings();
    return changed;
}

bool DecrementRandomizerCarCount(int panelIndex) {
    if (!IsThirtyCarModeEnabled()) {
        return false;
    }

    bool changed = RVGL_DecrementNumericMenuValue(panelIndex);
    SyncCarCountToVanillaSettings();
    return changed;
}

void SyncCarCountToVanillaSettings() {
    if (!IsThirtyCarModeEnabled()) {
        return;
    }

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

void Hook_DrawPreRaceSummary(int slotIndex) {
    if (!IsThirtyCarModeEnabled()) {
        Orig_DrawPreRaceSummary(slotIndex);
        return;
    }

    int& nativeCarCount = *reinterpret_cast<int*>(AbsFromRva(RVA_SETTINGS_NCARS));
    const int savedNativeCarCount = nativeCarCount;
    const int configuredCarCount = ClampRandomizerCarCount(
        GetRandomizerContext().carState.carsPerRace
    );

    if (configuredCarCount <= vanillaMaxCarCount) {
        Orig_DrawPreRaceSummary(slotIndex);
        return;
    }

    // DrawPreRaceSummary reads RaceSettings + 0x10 while composing the
    // "Racers: N (N-1 CPU)" text. Keep the native value capped for gameplay,
    // but expose the extended value for the duration of this draw call.
    nativeCarCount = configuredCarCount;
    Orig_DrawPreRaceSummary(slotIndex);
    nativeCarCount = savedNativeCarCount;
}

void PatchCarCountMenuDescriptor() {
    if (!IsThirtyCarModeEnabled()) {
        return;
    }

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

    if (!IsKnockoutModeEnabled()) {
        return;
    }

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

    if (!IsKnockoutModeEnabled()) {
        return;
    }

    PatchOptionsMenuLocaleStrings();
    InitializeModOptionsMenuItem();
    if (g_modOptionsMenuItemInitialized) {
        RVGL_RegisterMenuItemInActiveMenu(
            slotIndex,
            reinterpret_cast<int*>(&g_modOptionsMenuItem)
        );
    }
}

void Hook_BuildInRaceMenu(int slotIndex) {
    RestoreLocaleString(g_nextCarLabelOverride);
    Orig_BuildInRaceMenu(slotIndex);
    MenuPanelSlot* slot = GetMenuPanelSlot(slotIndex);
    const CarEntityRuntime* player = GetLocalPlayerCar();
    if (!CanSwitchCarsInRace() || slot == nullptr || player == nullptr ||
        slot->itemCount <= 0 || slot->itemCount >= 30) {
        return;
    }

    g_nextCarModelId = player->transform.modelId;
    if (!IsSelectableRaceCar(g_nextCarModelId) ||
        !OverrideLocaleString(g_nextCarLabelOverride)) {
        return;
    }

    const auto* nextTrackItem = GetInRaceNextTrackMenuItem();
    g_nextCarMenuValue.value = &g_nextCarModelId;
    g_nextCarMenuValue.minValue = 0;
    g_nextCarMenuValue.maxValue = GetRuntimeCarCount() - 1;
    g_nextCarMenuValue.step = 1;
    g_nextCarMenuItem = *nextTrackItem;
    g_nextCarMenuItem.labelId = kNextCarLabelId;
    g_nextCarMenuItem.valueWidth = 160.0f;
    g_nextCarMenuItem.valueBinding = &g_nextCarMenuValue;
    g_nextCarMenuItem.drawValue = DrawNextCar;
    g_nextCarMenuItem.decrementValue = DecrementNextCar;
    g_nextCarMenuItem.incrementValue = IncrementNextCar;

    RVGL_RegisterMenuItemInActiveMenu(slotIndex, reinterpret_cast<int*>(&g_nextCarMenuItem));
    // Keep the car selector beside Next Track rather than below Quit.
    const int lastItem = slot->itemCount - 1;
    for (int index = 0; index < lastItem; ++index) {
        if (slot->items[index] == nextTrackItem) {
            std::move_backward(slot->items + index + 1, slot->items + lastItem, slot->items + lastItem + 1);
            slot->items[index + 1] = &g_nextCarMenuItem;
            break;
        }
    }
}

bool Hook_HandleInRaceMenuAction(int slotIndex, uint32_t action) {
    if (GetSelectedMenuItemDescriptor(slotIndex) == &g_nextCarMenuItem &&
        action == kFrontendConfirmAction) {
        RaceSettingsRuntime* settings = GetRaceSettings();
        int* transitionRequest = GetRaceTransitionRequest();
        if (!CanSwitchCarsInRace() || !IsSelectableRaceCar(g_nextCarModelId) ||
            settings == nullptr || transitionRequest == nullptr || GetPlayerRaceInfo() == nullptr) {
            return false;
        }

        g_pendingInRaceCarModelId = g_nextCarModelId;
        // An explicit car choice must survive the native restart's random-car step.
        auto& gameMode = GetGameModeRuntime();
        gameMode.randomCars = 0;
        gameMode.randomTracks = 0;
        // Match the native immediate-restart action: fade out, save/tear down,
        // apply the pending car to PlayerRaceInfo, then reload the same level.
        *transitionRequest = 6;
        RVGL_ActivateMenuDisplaySlot(slotIndex, nullptr);
        RVGL_SetRaceState(3);
        Logger::TimestampLogf("[NextCar] Restarting with car %d: %s", g_nextCarModelId,
            GetCarPool()[g_nextCarModelId].internalName);
        RestoreLocaleString(g_nextCarLabelOverride);
        return true;
    }

    const bool result = Orig_HandleInRaceMenuAction(slotIndex, action);
    if (action == kFrontendCancelAction || action == kFrontendConfirmAction) {
        const MenuPanelSlot* slot = GetMenuPanelSlot(slotIndex);
        // The native pause menu can ignore Cancel or open a confirmation dialog.
        // Keep our label while the original panel remains displayed.
        if (slot == nullptr || slot->nextPanel != slot->panel) {
            RestoreLocaleString(g_nextCarLabelOverride);
        }
    }
    return result;
}

void ApplyPendingInRaceCarSelection() {
    const int modelId = g_pendingInRaceCarModelId;
    g_pendingInRaceCarModelId = -1;
    if (!IsSelectableRaceCar(modelId)) {
        return;
    }
    RaceSettingsRuntime* settings = GetRaceSettings();
    PlayerRaceInfoRuntime* raceInfo = GetPlayerRaceInfo();
    if (settings == nullptr || raceInfo == nullptr || raceInfo->participantCount <= 0) {
        return;
    }

    const CarInfo& car = GetCarPool()[modelId];
    // Solo restarts reuse the participant table at PlayerRaceInfo + 0x50.
    // Change it only after teardown has saved the OLD car's session/replay.
    auto* player = &raceInfo->participants[0];
    if (player->modelId == modelId) {
        return; // An unchanged selection retains the current car's skin.
    }
    player->modelId = modelId;
    player->skinId = 0;
    std::memcpy(player->carName, car.displayName, sizeof(player->carName));
    player->carName[sizeof(player->carName) - 1] = '\0';
    std::memset(player->skinName, 0, sizeof(player->skinName));
    if (car.pSkinNames != nullptr && car.skinCount > 0) {
        std::memcpy(player->skinName, car.pSkinNames, sizeof(player->skinName));
        player->skinName[sizeof(player->skinName) - 1] = '\0';
    }
    settings->playerModelIds[0] = modelId;
    settings->playerSkinId = 0;
}

// Called on every action on the "Options" menu (navigation with up/down arrows,
// choose option, cancel).
bool Hook_HandleOptionsMenuAction(int slotIndex, uint32_t action) {
    // The copied Mod Settings panel resolves its title through the same
    // locale slot as the Options-menu row. Keep that custom string loaded
    // when Confirm opens the panel; it is released when the panel is closed.
    const bool openingModOptions =
        action == kFrontendConfirmAction &&
        GetSelectedMenuItemDescriptor(slotIndex) == &g_modOptionsMenuItem;

    const bool result = Orig_HandleOptionsMenuAction(slotIndex, action);
    if (!IsKnockoutModeEnabled()) {
        return result;
    }

    if (action == kFrontendCancelAction ||
        (action == kFrontendConfirmAction && !openingModOptions)) {
        RestoreOptionsMenuLocaleStrings();
    }

    return result;
}

// Called on every action on the "Start Race" menu (navigation with up/down arrows, choose option, cancel)
bool Hook_HandleStartRaceMenuAction(int slotIndex, uint32_t action) {
    if (!IsKnockoutModeEnabled()) {
        ClearKnockoutMenuSelection();
        return Orig_HandleStartRaceMenuAction(slotIndex, action);
    }

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
