#pragma once

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
    using FnLoadVanillaCarPool  = bool(*)();
    using FnLoadCustomCarPool   = void(*)();
    using FnLoadTextureByName   = unsigned long long(*)(char* path, int slotID, int maxMipLevel, bool enableMips, int param_5, unsigned int flags);

    // ------------------------------------------------------------------------
    // Original function pointers.
    // Declared here, defined in Randomizer.cpp.
    // MinHook writes the trampoline addresses into these during InstallAll().
    // Call these from inside the detours to invoke the real RVGL functions.
    // ------------------------------------------------------------------------
    extern FnLoadVanillaCarPool  Orig_LoadVanillaCarPool;
    extern FnLoadCustomCarPool   Orig_LoadCustomCarPool;
    extern FnLoadTextureByName   Orig_LoadTextureByName;

    // ------------------------------------------------------------------------
    // Detour functions — registered in HookManager.cpp → RegisterHooks().
    // ------------------------------------------------------------------------
    bool Hook_LoadVanillaCarPool();
    void Hook_LoadCustomCarPool();
    unsigned long long Hook_LoadTextureByName(char* path, int slotID, int maxMipLevel, bool enableMips, int param_5, unsigned int flags);

} // namespace Randomizer
