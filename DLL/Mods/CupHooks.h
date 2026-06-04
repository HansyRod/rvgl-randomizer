#pragma once
#include "RVGLStructs.h"
#include <cstdio>

namespace Randomizer {

    struct RandomizedCup;

    // ------------------------------------------------------------------------
    // Function pointer types — must exactly match the RVGL originals.
    // Ghidra signatures (x64, __fastcall is noise and is dropped):
    //   LoadVanillaCarPool:  bool ()
    // ------------------------------------------------------------------------
    using FnLoadVanillaCups         = void(*)();
    using FnLoadCustomCups          = void(*)();
    using FnCup_ValidateAndCheckUnlock = void(*)(int cupID);

    // ------------------------------------------------------------------------
    // Original function pointers.
    // Declared here, defined in Randomizer.cpp.
    // MinHook writes the trampoline addresses into these during InstallAll().
    // Call these from inside the detours to invoke the real RVGL functions.
    // ------------------------------------------------------------------------
    extern FnLoadVanillaCups        Orig_LoadVanillaCups;
    extern FnLoadCustomCups         Orig_LoadCustomCups;
    extern FnCup_ValidateAndCheckUnlock Orig_Cup_ValidateAndCheckUnlock;
    // ------------------------------------------------------------------------
    // Detour functions — registered in HookManager.cpp → RegisterHooks().
    // ------------------------------------------------------------------------
    void Hook_LoadVanillaCups();
    void Hook_LoadCustomCups();
    void Hook_Cup_ValidateAndCheckUnlock(int cupID);
    CupProfile* GetCupProfileByCupID(int cupID);
    RandomizedCup* GetCupConfigByCupID(int cupID);

} // namespace Randomizer
