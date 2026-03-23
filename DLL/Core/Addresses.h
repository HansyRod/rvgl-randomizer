#pragma once
#include <cstdint>
#include <windows.h>

// ============================================================================
// Addresses.h
//
// RVA constants for all hooked and read functions/globals in RVGL.
// These are relative virtual addresses — offsets from the start of rvgl.exe.
//
// To update after an RVGL update:
//   1. Open the new rvgl.exe in Ghidra.
//   2. Find each function listed below.
//   3. Replace the constant with the new RVA (address shown in Ghidra
//      minus the image base, which is 0x00400000 for RVGL).
// ============================================================================

// ----------------------------------------------------------------------------
// Functions
// ----------------------------------------------------------------------------

// Car_LoadVanillaPool — loads the built-in car pool into memory.
// Ghidra signature: bool __fastcall Car_LoadVanillaPool(void)
constexpr uint32_t RVA_LOAD_VANILLA_CAR_POOL   = 0x0003F140;

// LoadTextureByName - receives a file path and loads a texture to a given slot id.
constexpr uint32_t RVA_LOAD_TEXTURE_BY_NAME = 0x000962d0;

// ----------------------------------------------------------------------------
// Globals
// ----------------------------------------------------------------------------

// Pointer to the CarInfo array (DAT_006fab50).
// Dereference to get the base address of the car pool.
constexpr uint32_t RVA_CAR_TABLE   = 0x002FAB50;

// Number of cars currently in the pool (DAT_006fab58).
// Includes both vanilla (49) and any loaded custom cars.
constexpr uint32_t RVA_CAR_COUNT   = 0x002FAB58;

// ----------------------------------------------------------------------------
// AbsFromRva
//
// Converts a compile-time RVA constant to the actual runtime address by
// adding the base address RVGL was loaded at. Call this at hook installation
// time — not at compile time — because the module base is only known once
// the process is running.
// ----------------------------------------------------------------------------
inline uintptr_t AbsFromRva(uint32_t rva) {
    return reinterpret_cast<uintptr_t>(GetModuleHandleA("rvgl.exe")) + rva;
}
