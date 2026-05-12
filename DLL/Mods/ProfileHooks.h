#pragma once
#include "RVGLStructs.h"
#include <cstdio>

namespace Randomizer {

    // ------------------------------------------------------------------------
    // Function pointer types — must exactly match the RVGL originals.
    // Ghidra signatures (x64, __fastcall is noise and is dropped):
    //   LoadVanillaCarPool:  bool ()
    // ------------------------------------------------------------------------
    using FnGetProfileIndex         = int(*)(char* profileName);
    using FnProfile_CreateOrLoad    = bool(*)(char* displayName);
    using FnProfile_LoadAndReset    = void(*)(char* profileName);
    using FnLoadSettingsFromIni     = void(*)(char* profileName);

    // ------------------------------------------------------------------------
    // Original function pointers.
    // Declared here, defined in Randomizer.cpp.
    // MinHook writes the trampoline addresses into these during InstallAll().
    // Call these from inside the detours to invoke the real RVGL functions.
    // ------------------------------------------------------------------------
    extern FnGetProfileIndex        Orig_GetProfileIndex;
    extern FnProfile_CreateOrLoad   Orig_Profile_CreateOrLoad;
    extern FnProfile_LoadAndReset   Orig_Profile_LoadAndReset;
    extern FnLoadSettingsFromIni    Orig_LoadSettingsFromIni;

    // ------------------------------------------------------------------------
    // Detour functions — registered in HookManager.cpp → RegisterHooks().
    // ------------------------------------------------------------------------
    int Hook_GetProfileIndex(char* profileName);
    bool Hook_Profile_CreateOrLoad(char* displayName);
    void Hook_Profile_LoadAndReset(char* profileName);
    void Hook_LoadSettingsFromIni(char* profileName);

} // namespace Randomizer
