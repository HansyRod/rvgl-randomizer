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
    using FnLoadSettingsFromIni     = void(*)(char* profileName);
    using FnTrack_ApplyCustomUnlock = void(*)(int trackIndex);
    using FnCheckIfTierChampionshipWon = bool(*)(int difficulty);
    using FnCheckIfTierTimeTrialsBeaten = bool(*)(int difficulty);
    using FnCheckIfTierPracticeStarsFound = bool(*)(int difficulty);
    using FnCheckIfTierSingleRacesWon = bool(*)(int difficulty);
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
    extern FnLoadVanillaTracks      Orig_LoadVanillaTracks;
    extern FnLoadCustomTracks       Orig_LoadCustomTracks;
    extern FnLoadVanillaCups        Orig_LoadVanillaCups;
    extern FnLoadCustomCups         Orig_LoadCustomCups;
    extern FnGetProfileIndex        Orig_GetProfileIndex;
    extern FnProfile_CreateOrLoad   Orig_Profile_CreateOrLoad;
    extern FnProfile_LoadAndReset   Orig_Profile_LoadAndReset;
    extern FnLoadSettingsFromIni    Orig_LoadSettingsFromIni;
    extern FnTrack_ApplyCustomUnlock Orig_Track_ApplyCustomUnlock;
    extern FnCheckIfTierChampionshipWon Orig_CheckIfTierChampionshipWon;
    extern FnCheckIfTierTimeTrialsBeaten Orig_CheckIfTierTimeTrialsBeaten;
    extern FnCheckIfTierPracticeStarsFound Orig_CheckIfTierPracticeStarsFound;
    extern FnCheckIfTierSingleRacesWon Orig_CheckIfTierSingleRacesWon;
    extern FnInitCarPhysicsBlock    Orig_InitCarPhysicsBlock;
    extern FnToken_Matches          Orig_Token_Matches;
    extern FnReadTokenFloat         Orig_ReadTokenFloat;
    extern FnReadTokenInt           Orig_ReadTokenInt;
    extern FnReadTokenBool          Orig_ReadTokenBool;

    // ------------------------------------------------------------------------
    // Detour functions — registered in HookManager.cpp → RegisterHooks().
    // ------------------------------------------------------------------------
    void Hook_LoadVanillaTracks();
    void Hook_LoadCustomTracks();
    void Hook_LoadVanillaCups();
    void Hook_LoadCustomCups();
    int Hook_GetProfileIndex(char* profileName);
    bool Hook_Profile_CreateOrLoad(char* displayName);
    void Hook_Profile_LoadAndReset(char* profileName);
    void Hook_LoadSettingsFromIni(char* profileName);
    void Hook_Track_ApplyCustomUnlock(int trackIndex);
    bool Hook_CheckIfTierChampionshipWon(int difficulty);
    bool Hook_CheckIfTierTimeTrialsBeaten(int difficulty);
    bool Hook_CheckIfTierPracticeStarsFound(int difficulty);
    bool Hook_CheckIfTierSingleRacesWon(int difficulty);
    void Hook_InitCarPhysicsBlock(CarPhysicsData *physData, int carIndex);
    bool Hook_Token_Matches(char* str, char* pattern);
    bool Hook_ReadTokenFloat(float* outValue, FILE* file);
    bool Hook_ReadTokenInt(int* outValue, FILE* file);
    bool Hook_ReadTokenBool(bool* outValue, FILE* file);

    // Utils functions
    void ApplyStockTrackData(TrackInfo* track);
} // namespace Randomizer
