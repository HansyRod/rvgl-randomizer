#pragma once
#include "RVGLStructs.h"
#include <cstdio>
#include <string>

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
    using FnLoadVanillaTracks       = void(*)();
    using FnLoadCustomTracks        = void(*)();
    using FnTrack_ApplyCustomUnlock = void(*)(int trackIndex);
    using FnCheckIfTierChampionshipWon = bool(*)(int difficulty);
    using FnCheckIfTierTimeTrialsBeaten = bool(*)(int difficulty);
    using FnCheckIfTierPracticeStarsFound = bool(*)(int difficulty);
    using FnCheckIfTierSingleRacesWon = bool(*)(int difficulty);

    // ------------------------------------------------------------------------
    // Original function pointers.
    // Declared here, defined in Randomizer.cpp.
    // MinHook writes the trampoline addresses into these during InstallAll().
    // Call these from inside the detours to invoke the real RVGL functions.
    // ------------------------------------------------------------------------
    extern FnLoadVanillaTracks      Orig_LoadVanillaTracks;
    extern FnLoadCustomTracks       Orig_LoadCustomTracks;
    extern FnTrack_ApplyCustomUnlock Orig_Track_ApplyCustomUnlock;
    extern FnCheckIfTierChampionshipWon Orig_CheckIfTierChampionshipWon;
    extern FnCheckIfTierTimeTrialsBeaten Orig_CheckIfTierTimeTrialsBeaten;
    extern FnCheckIfTierPracticeStarsFound Orig_CheckIfTierPracticeStarsFound;
    extern FnCheckIfTierSingleRacesWon Orig_CheckIfTierSingleRacesWon;

    // ------------------------------------------------------------------------
    // Detour functions — registered in HookManager.cpp → RegisterHooks().
    // ------------------------------------------------------------------------
    void Hook_LoadVanillaTracks();
    void Hook_LoadCustomTracks();
    void Hook_Track_ApplyCustomUnlock(int trackIndex);
    bool Hook_CheckIfTierChampionshipWon(int difficulty);
    bool Hook_CheckIfTierTimeTrialsBeaten(int difficulty);
    bool Hook_CheckIfTierPracticeStarsFound(int difficulty);
    bool Hook_CheckIfTierSingleRacesWon(int difficulty);

    // Utils functions
    int FindTrackIdByFolderName(const char* trackName);
    void ApplyStockTrackData(TrackInfo* track);

    int GetRuntimeTrackCount();
    TrackInfo* GetVanillaTrackArray();
    TrackInfo* GetCustomTrackArray();
    TrackInfo* GetTrackInfoByRuntimeIndex(int trackIndex);
    int FindTrackIdByFolderName(const std::string& folderName);

} // namespace Randomizer
