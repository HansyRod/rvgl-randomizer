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

// MSVC allocates these fields from the low bit upwards in RVGL's four-byte flags word.
struct MenuItemFlags {
    uint32_t selectable : 1;
    uint32_t enabled : 1;
    uint32_t otherFlags : 30;
};

struct MenuItemDescriptor {
    int labelId;                                // +0x00
    MenuItemFlags flags;                        // +0x04
    float valueWidth;                           // +0x08
    int reserved0C;                             // +0x0C
    NumericMenuValue* valueBinding;             // +0x10
    Randomizer::FnDrawNumericMenuValue drawValue;           // +0x18
    Randomizer::FnDecrementNumericMenuValue decrementValue; // +0x20
    Randomizer::FnIncrementNumericMenuValue incrementValue; // +0x28
    void* reserved30;                           // +0x30
    void* reserved38;                           // +0x38
};

// Only fields used by the connection menu are named. The rest stays byte-for-byte
// as RVGL supplied it when copying the native Join Game panel.
struct MenuPanelDescriptor {
    uint32_t flags;                             // +0x00
    int titleLabel;                             // +0x04
    uint8_t _pad08[0x18];
    void (*build)(int slotIndex);                // +0x20
    bool (*handleAction)(int slotIndex, uint32_t action); // +0x28
    uint8_t _pad30[8];
    int selectedItem;                           // +0x38
    uint8_t _pad3C[4];
    MenuPanelDescriptor* parent;                // +0x40
    uint8_t _pad48[0x18];
};

struct MenuPanelSlot {
    MenuPanelDescriptor* panel;                 // +0x00, currently displayed
    MenuPanelDescriptor* nextPanel;             // +0x08, destination after transition
    MenuItemDescriptor* items[30];              // +0x10
    int16_t itemCount;                          // +0x100
    int16_t visibleRowCount;                    // +0x102
    int state;                                 // +0x104, 1 = settled, 2 = leaving
    uint8_t _pad108[0x0C];
    float textX;                               // +0x114
    float textY;                               // +0x118
    uint8_t _pad11C[0x18];
    float labelWidth;                          // +0x134
    float valueWidth;                          // +0x138
    uint8_t _pad13C[4];
};

#pragma pack(pop)

static_assert(sizeof(MenuPanelDescriptor) == 0x60, "MenuPanelDescriptor size mismatch.");
static_assert(offsetof(MenuPanelDescriptor, parent) == 0x40, "MenuPanelDescriptor::parent offset mismatch.");
static_assert(sizeof(MenuPanelSlot) == 0x140, "MenuPanelSlot size mismatch.");
static_assert(offsetof(MenuPanelSlot, textX) == 0x114, "MenuPanelSlot::textX offset mismatch.");

static_assert(sizeof(NumericMenuValue) == 0x20, "NumericMenuValue size mismatch.");
static_assert(offsetof(NumericMenuValue, value) == 0x00, "NumericMenuValue::value offset mismatch.");
static_assert(offsetof(NumericMenuValue, minValue) == 0x08, "NumericMenuValue::minValue offset mismatch.");
static_assert(offsetof(NumericMenuValue, maxValue) == 0x0C, "NumericMenuValue::maxValue offset mismatch.");
static_assert(offsetof(NumericMenuValue, step) == 0x10, "NumericMenuValue::step offset mismatch.");
static_assert(offsetof(NumericMenuValue, useSlider) == 0x14, "NumericMenuValue::useSlider offset mismatch.");
static_assert(sizeof(MenuItemDescriptor) == 0x40, "MenuItemDescriptor size mismatch.");
static_assert(sizeof(MenuItemFlags) == 4, "MenuItemFlags size mismatch.");
static_assert(offsetof(MenuItemDescriptor, flags) == 0x04, "MenuItemDescriptor::flags offset mismatch.");
static_assert(offsetof(MenuItemDescriptor, valueWidth) == 0x08, "MenuItemDescriptor::valueWidth offset mismatch.");
static_assert(offsetof(MenuItemDescriptor, valueBinding) == 0x10, "MenuItemDescriptor::valueBinding offset mismatch.");
static_assert(offsetof(MenuItemDescriptor, drawValue) == 0x18, "MenuItemDescriptor::drawValue offset mismatch.");
static_assert(offsetof(MenuItemDescriptor, decrementValue) == 0x20, "MenuItemDescriptor::decrementValue offset mismatch.");
static_assert(offsetof(MenuItemDescriptor, incrementValue) == 0x28, "MenuItemDescriptor::incrementValue offset mismatch.");
