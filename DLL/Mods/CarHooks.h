#pragma once
#include "RVGLStructs.h"
#include "ConfigData.h"
#include <cstdio>

// ============================================================================
// Randomizer
//
// Hooks into the car loading pipeline to randomize the car selection.
//
// Hook_LoadVanillaCarPool  — fires after RVGL builds the vanilla car pool. 
//                 Used to snapshot which cars are available for selection.
// ============================================================================

namespace Randomizer {

    // ------------------------------------------------------------------------
    // Function pointer types — must exactly match the RVGL originals.
    // Ghidra signatures (x64, __fastcall is noise and is dropped):
    //   LoadVanillaCarPool:  bool ()
    // ------------------------------------------------------------------------
    using FnLoadVanillaCarPool      = bool(*)();
    using FnLoadCustomCarPool       = void(*)();
    using FnLoadTextureByName       = unsigned long long(*)(char* path, int slotID, int maxMipLevel, bool enableMips, int param_5, unsigned int flags);
    using FnSyncCarInfoFromPhysics  = void(*)(int carIndex, CarPhysicsData *physData);
    using FnUpdateCarSelectability  = void(*)();

    // ------------------------------------------------------------------------
    // Original function pointers.
    // Declared here, defined in Randomizer.cpp.
    // MinHook writes the trampoline addresses into these during InstallAll().
    // Call these from inside the detours to invoke the real RVGL functions.
    // ------------------------------------------------------------------------
    extern FnLoadVanillaCarPool     Orig_LoadVanillaCarPool;
    extern FnLoadCustomCarPool      Orig_LoadCustomCarPool;
    extern FnLoadTextureByName      Orig_LoadTextureByName;
    extern FnSyncCarInfoFromPhysics Orig_SyncCarInfoFromPhysics;
    extern FnUpdateCarSelectability Orig_UpdateCarSelectability;

    // ------------------------------------------------------------------------
    // Detour functions — registered in HookManager.cpp → RegisterHooks().
    // ------------------------------------------------------------------------
    bool Hook_LoadVanillaCarPool();
    void Hook_LoadCustomCarPool();
    unsigned long long Hook_LoadTextureByName(char* path, int slotID, int maxMipLevel, bool enableMips, int param_5, unsigned int flags);
    void Hook_SyncCarInfoFromPhysics(int carIndex, CarPhysicsData *physData);
    void Hook_UpdateCarSelectability();

    // Utils functions
    void ApplyCarMods(int carIndex, CarInfo* car, CarPhysicsData *physData);
    RandomizedCar* GetCarConfigByRuntimeIndex(int carIndex);
    int GetRuntimeCarCount();

    // Returns the live RVGL car array. RVA_CAR_TABLE stores a pointer, so this
    // helper intentionally dereferences the global pointer slot.
    CarInfo* GetCarPool();

} // namespace Randomizer
