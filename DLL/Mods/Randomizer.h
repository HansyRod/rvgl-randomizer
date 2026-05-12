#pragma once
#include "RVGLStructs.h"
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
    using FnLoadVanillaCups         = void(*)();
    using FnLoadCustomCups          = void(*)();
    using FnUpdateCarSelectability  = void(*)();
    using FnGetProfileIndex         = int(*)(char* profileName);
    using FnProfile_CreateOrLoad    = bool(*)(char* displayName);
    using FnProfile_LoadAndReset    = void(*)(char* profileName);
    using FnLoadSettingsFromIni     = void(*)(char* profileName);
    using FnInitCarPhysicsBlock = void(*)(CarPhysicsData *physData, int carIndex);
    using FnToken_Matches       = bool(*)(char* str, char* pattern);
    using FnReadTokenFloat      = bool(*)(float* outValue, FILE* file);
    using FnReadTokenInt        = bool(*)(int* outValue, FILE* file);
    using FnReadTokenBool       = bool(*)(bool* outValue, FILE* file);

    // ------------------------------------------------------------------------
    // Original function pointers.
    // Declared here, defined in Randomizer.cpp.
    // MinHook writes the trampoline addresses into these during InstallAll().
    // Call these from inside the detours to invoke the real RVGL functions.
    // ------------------------------------------------------------------------
    extern FnLoadVanillaCups        Orig_LoadVanillaCups;
    extern FnLoadCustomCups         Orig_LoadCustomCups;
    extern FnGetProfileIndex        Orig_GetProfileIndex;
    extern FnProfile_CreateOrLoad   Orig_Profile_CreateOrLoad;
    extern FnProfile_LoadAndReset   Orig_Profile_LoadAndReset;
    extern FnLoadSettingsFromIni    Orig_LoadSettingsFromIni;
    extern FnInitCarPhysicsBlock    Orig_InitCarPhysicsBlock;
    extern FnToken_Matches          Orig_Token_Matches;
    extern FnReadTokenFloat         Orig_ReadTokenFloat;
    extern FnReadTokenInt           Orig_ReadTokenInt;
    extern FnReadTokenBool          Orig_ReadTokenBool;

    // ------------------------------------------------------------------------
    // Detour functions — registered in HookManager.cpp → RegisterHooks().
    // ------------------------------------------------------------------------
    void Hook_LoadVanillaCups();
    void Hook_LoadCustomCups();
    int Hook_GetProfileIndex(char* profileName);
    bool Hook_Profile_CreateOrLoad(char* displayName);
    void Hook_Profile_LoadAndReset(char* profileName);
    void Hook_LoadSettingsFromIni(char* profileName);
    void Hook_InitCarPhysicsBlock(CarPhysicsData *physData, int carIndex);
    bool Hook_Token_Matches(char* str, char* pattern);
    bool Hook_ReadTokenFloat(float* outValue, FILE* file);
    bool Hook_ReadTokenInt(int* outValue, FILE* file);
    bool Hook_ReadTokenBool(bool* outValue, FILE* file);

} // namespace Randomizer
