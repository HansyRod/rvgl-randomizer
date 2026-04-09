#pragma once
#include "RVGLStructs.h"

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
    using FnLoadVanillaTracks       = void(*)();
    using FnLoadCustomTracks        = void(*)();
    using FnLoadVanillaCups         = void(*)();
    using FnLoadCustomCups          = void(*)();
    using FnUpdateCarSelectability  = void(*)();
    using FnGetProfileIndex         = int(*)(char* profileName);
    using FnProfile_CreateOrLoad    = bool(*)(char* displayName);
    using FnProfile_LoadAndReset    = void(*)(char* profileName);

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
    extern FnLoadVanillaTracks      Orig_LoadVanillaTracks;
    extern FnLoadCustomTracks       Orig_LoadCustomTracks;
    extern FnLoadVanillaCups        Orig_LoadVanillaCups;
    extern FnLoadCustomCups         Orig_LoadCustomCups;
    extern FnUpdateCarSelectability Orig_UpdateCarSelectability;
    extern FnGetProfileIndex        Orig_GetProfileIndex;
    extern FnProfile_CreateOrLoad   Orig_Profile_CreateOrLoad;
    extern FnProfile_LoadAndReset   Orig_Profile_LoadAndReset;

    // ------------------------------------------------------------------------
    // Detour functions — registered in HookManager.cpp → RegisterHooks().
    // ------------------------------------------------------------------------
    bool Hook_LoadVanillaCarPool();
    void Hook_LoadCustomCarPool();
    unsigned long long Hook_LoadTextureByName(char* path, int slotID, int maxMipLevel, bool enableMips, int param_5, unsigned int flags);
    void Hook_SyncCarInfoFromPhysics(int carIndex, CarPhysicsData *physData);
    void Hook_LoadVanillaTracks();
    void Hook_LoadCustomTracks();
    void Hook_LoadVanillaCups();
    void Hook_LoadCustomCups();
    void Hook_UpdateCarSelectability();
    int Hook_GetProfileIndex(char* profileName);
    bool Hook_Profile_CreateOrLoad(char* displayName);
    void Hook_Profile_LoadAndReset(char* profileName);

    // Utils functions
    void Initialize();
    void ApplyCarMods(int carIndex, CarInfo* car, CarPhysicsData *physData);
    void ApplyStockTrackData(TrackInfo* track);
} // namespace Randomizer
