#pragma once

#include <cstddef>
#include <cstdint>
#include "RVGLFunctions.h"

// ============================================================================
// RVGLMenuStructs.h
//
// Shared RVGL runtime layouts used by frontend menu code.
// Keep these in Game/ so they are declared once and reused everywhere.
// ============================================================================

#pragma pack(push, 1)

struct NumericMenuValue {
    int* value;                                 // +0x00
    int minValue;                               // +0x08
    int maxValue;                               // +0x0C
    int step;                                   // +0x10
    bool useSlider;                             // +0x14
    uint8_t _pad_15[11];
};

struct MenuItemDescriptor {
    int labelId;                                // +0x00
    int flags;                                  // +0x04
    int32_t valueWidth;                         // +0x08
    int reserved0C;                             // +0x0C
    NumericMenuValue* valueBinding;             // +0x10
    Randomizer::FnDrawNumericMenuValue drawValue;           // +0x18
    Randomizer::FnDecrementNumericMenuValue decrementValue; // +0x20
    Randomizer::FnIncrementNumericMenuValue incrementValue; // +0x28
    void* reserved30;                           // +0x30
    void* reserved38;                           // +0x38
};

#pragma pack(pop)

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
