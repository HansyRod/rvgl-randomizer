#pragma once
#include "RVGLStructs.h"
#include <cstdio>
#include <cstdint>
#include <set>
#include <string>

namespace Randomizer {

    void SaveArchipelagoProfileConnection(const std::string& server, bool requiresPassword);
    void MarkArchipelagoProfilePasswordRequired();
    // Completed checks awaiting server confirmation, scoped to a seed and slot within the active RVGL profile.
    void RecordArchipelagoProfileLocationCheck(const std::string& seed, int slot, int64_t locationId,
        bool goalLocation = false);
    std::set<int64_t> GetArchipelagoProfileLocationChecks(const std::string& seed, int slot);
    void RemoveConfirmedArchipelagoProfileLocationChecks(
        const std::string& seed, int slot, const std::set<int64_t>& confirmedLocations);
    // Goal history survives confirmation and is stored separately from pending checks.
    void RecordArchipelagoProfileGoalLocations(const std::string& seed, int slot,
        const std::set<int64_t>& locationIds);
    const std::set<int64_t>* GetArchipelagoProfileGoalLocations(const std::string& seed, int slot);
    // Uses list of car folders, persisted in the Archipelago profile data.
    void RecordArchipelagoProfileCarWin(const std::string& seed, int slot, const std::string& carFolder);
    // Returns stored wins without copying, or nullptr if the seed/slot is missing.
    const std::set<std::string>* GetArchipelagoProfileCarWins(const std::string& seed, int slot);
    void SaveArchipelagoProfileReceivedItems(const std::string& seed, int slot);
    void FlushArchipelagoProfileData();

    // ------------------------------------------------------------------------
    // Function pointer types — must exactly match the RVGL originals.
    // Ghidra signatures (x64, __fastcall is noise and is dropped):
    //   LoadVanillaCarPool:  bool ()
    // ------------------------------------------------------------------------
    using FnGetProfileIndex         = int(*)(char* profileName);
    using FnProfile_CreateOrLoad    = bool(*)(char* displayName);
    using FnProfile_LoadAndReset    = void(*)(char* profileName);
    using FnLoadSettingsFromIni     = void(*)(char* profileName);
    using FnIni_SaveProfile         = bool(*)(char* profileName);

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
    extern FnIni_SaveProfile        Orig_Ini_SaveProfile;

    // ------------------------------------------------------------------------
    // Detour functions — registered in HookManager.cpp → RegisterHooks().
    // ------------------------------------------------------------------------
    int Hook_GetProfileIndex(char* profileName);
    bool Hook_Profile_CreateOrLoad(char* displayName);
    void Hook_Profile_LoadAndReset(char* profileName);
    void Hook_LoadSettingsFromIni(char* profileName);
    bool Hook_Ini_SaveProfile(char* profileName);

} // namespace Randomizer
