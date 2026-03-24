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

// LoadVanillaCarPool — loads the built-in car pool into memory.
// Ghidra signature: bool __fastcall LoadVanillaCarPool(void)
constexpr uint32_t RVA_LOAD_VANILLA_CAR_POOL   = 0x0003F140;

// LoadCustomCarPool — loads the custom car pool into memory.
// Ghidra signature: void LoadCustomCarPool (void)
constexpr uint32_t RVA_LOAD_CUSTOM_CAR_POOL = 0x0003fac0;

// CreateCarbox — sets up the frontend textures for car selection.
// Ghidra signature: void __fastcall CreateCarbox(void* param_1)
constexpr uint32_t RVA_CREATE_CARBOX = 0x001570d0;

// LoadTextureByName - receives a file path and loads a texture to a given slot id.
constexpr uint32_t RVA_LOAD_TEXTURE_BY_NAME = 0x000962d0;

// SyncCarInfoFromPhysics - Runs when a car instance is initialized,
// copies parameters back from physics object into CarInfo.
constexpr uint32_t RVA_SYNC_CAR_INFO_FROM_PHYSICS = 0x0003c330;

// ParseParametersTxt - Parses the basic metadata when loading a car.
constexpr uint32_t RVA_PARSE_PARAMETERS_TXT = 0x0003b6c0;

// ----------------------------------------------------------------------------
// Globals
// ----------------------------------------------------------------------------

// Pointer to the CarInfo array (DAT_006fab50).
// Dereference to get the base address of the car pool.
constexpr uint32_t RVA_CAR_TABLE   = 0x002FAB50;

// Number of cars currently in the pool (DAT_006fab58).
// Includes both vanilla (49) and any loaded custom cars.
constexpr uint32_t RVA_CAR_COUNT   = 0x002FAB58;

// Pointer table of 49 "cars/<name>" path strings used by LoadVanillaCarPool.
// Each entry points to a null-terminated string in .rdata.
// Override pointers before LoadVanillaCarPool to replace base cars.
constexpr uint32_t RVA_VANILLA_CAR_PATHS = 0x002720a0;

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
