#include "MenuMod.h"
#include "Addresses.h"
#include "30CarMod.h"
#include "RandomizerState.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>

// ============================================================================
// MenuMod.cpp
//
// Hooks for modifying menu interaction
// ============================================================================

namespace Randomizer {

namespace {

constexpr int kMinCarCount = 2;
constexpr int kRandomizerMaxCarCount = 30;

struct NumericMenuValue {
    int* value;
    int minValue;
    int maxValue;
    int step;
    bool useSlider;
    uint8_t padding[11];
};

struct MenuItemDescriptor {
    int labelId;
    int flags;
    float valueWidth;
    int reserved0C;
    NumericMenuValue* valueBinding;
    FnDrawNumericMenuValue drawValue;
    FnDecrementNumericMenuValue decrementValue;
    FnIncrementNumericMenuValue incrementValue;
    void* reserved30;
    void* reserved38;
};

static_assert(sizeof(NumericMenuValue) == 0x20, "NumericMenuValue size mismatch.");
static_assert(offsetof(NumericMenuValue, value) == 0x00, "NumericMenuValue::value offset mismatch.");
static_assert(offsetof(NumericMenuValue, minValue) == 0x08, "NumericMenuValue::minValue offset mismatch.");
static_assert(offsetof(NumericMenuValue, maxValue) == 0x0C, "NumericMenuValue::maxValue offset mismatch.");
static_assert(offsetof(NumericMenuValue, step) == 0x10, "NumericMenuValue::step offset mismatch.");
static_assert(offsetof(NumericMenuValue, useSlider) == 0x14, "NumericMenuValue::useSlider offset mismatch.");
static_assert(sizeof(MenuItemDescriptor) == 0x40, "MenuItemDescriptor size mismatch.");
static_assert(offsetof(MenuItemDescriptor, valueBinding) == 0x10, "MenuItemDescriptor::valueBinding offset mismatch.");
static_assert(offsetof(MenuItemDescriptor, drawValue) == 0x18, "MenuItemDescriptor::drawValue offset mismatch.");
static_assert(offsetof(MenuItemDescriptor, decrementValue) == 0x20, "MenuItemDescriptor::decrementValue offset mismatch.");
static_assert(offsetof(MenuItemDescriptor, incrementValue) == 0x28, "MenuItemDescriptor::incrementValue offset mismatch.");

NumericMenuValue g_carCountMenuValue = {};

void EnsureNumericMenuFunctionsResolved() {
    if (Orig_DrawNumericMenuValue == nullptr) {
        Orig_DrawNumericMenuValue = reinterpret_cast<FnDrawNumericMenuValue>(
            AbsFromRva(RVA_DRAW_NUMERIC_MENU_VALUE)
        );
    }

    if (Orig_DecrementNumericMenuValue == nullptr) {
        Orig_DecrementNumericMenuValue = reinterpret_cast<FnDecrementNumericMenuValue>(
            AbsFromRva(RVA_DECREMENT_NUMERIC_MENU_VALUE)
        );
    }

    if (Orig_IncrementNumericMenuValue == nullptr) {
        Orig_IncrementNumericMenuValue = reinterpret_cast<FnIncrementNumericMenuValue>(
            AbsFromRva(RVA_INCREMENT_NUMERIC_MENU_VALUE)
        );
    }
}

int ClampRandomizerCarCount(int carCount) {
    return std::clamp(carCount, kMinCarCount, kRandomizerMaxCarCount);
}

} // anonymous namespace

// ----------------------------------------------------------------------------
// RVGL numeric menu functions
// ----------------------------------------------------------------------------
FnDrawNumericMenuValue          Orig_DrawNumericMenuValue       = nullptr;
FnDecrementNumericMenuValue     Orig_DecrementNumericMenuValue  = nullptr;
FnIncrementNumericMenuValue     Orig_IncrementNumericMenuValue  = nullptr;

bool IncrementRandomizerCarCount(int panelIndex) {
    EnsureNumericMenuFunctionsResolved();

    bool changed = Orig_IncrementNumericMenuValue(panelIndex);
    SyncCarCountToVanillaSettings();
    return changed;
}

bool DecrementRandomizerCarCount(int panelIndex) {
    EnsureNumericMenuFunctionsResolved();

    bool changed = Orig_DecrementNumericMenuValue(panelIndex);
    SyncCarCountToVanillaSettings();
    return changed;
}

void PatchCarCountMenuDescriptor() {
    EnsureNumericMenuFunctionsResolved();

    RandomizerContext& ctx = GetRandomizerContext();
    ctx.carState.carsPerRace = ClampRandomizerCarCount(ctx.carState.carsPerRace);

    g_carCountMenuValue.value = &ctx.carState.carsPerRace;
    g_carCountMenuValue.minValue = kMinCarCount;
    g_carCountMenuValue.maxValue = kRandomizerMaxCarCount;
    g_carCountMenuValue.step = 1;
    g_carCountMenuValue.useSlider = false;

    auto* carCountMenuItem = reinterpret_cast<MenuItemDescriptor*>(
        AbsFromRva(RVA_SETTINGS_NCARS_MENU_ITEM)
    );

    carCountMenuItem->valueBinding = &g_carCountMenuValue;
    carCountMenuItem->drawValue = Orig_DrawNumericMenuValue;
    carCountMenuItem->decrementValue = DecrementRandomizerCarCount;
    carCountMenuItem->incrementValue = IncrementRandomizerCarCount;

    SyncCarCountToVanillaSettings();
}

} // namespace Randomizer
