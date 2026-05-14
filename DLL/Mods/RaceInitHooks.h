#pragma once
#include "RVGLStructs.h"
#include <cstdio>

namespace Randomizer {

    // ------------------------------------------------------------------------
    // Function pointer types — must exactly match the RVGL originals.
    // Ghidra signatures (x64, __fastcall is noise and is dropped):
    //   LoadVanillaCarPool:  bool ()
    // ------------------------------------------------------------------------
    using FnDrawPostRaceLeaderboard = void(*)();
    using FnRaceSessionSetup = void(*)(bool isRestart);
    using FnAssignStartPositions = void(*)();
    using FnSetupAllRaceCars = void(*)();
    using FnRandomizeCarPicks = void(*)();
    using FnAddParticipantAndCount = bool (*)(int carType, int spawnType, int carID, int skinID, int isLocal, int networkID, char *playerName);
    using FnUpdateRacePositions = void(*)();

    // ------------------------------------------------------------------------
    // Original function pointers.
    // Declared here, defined in Randomizer.cpp.
    // MinHook writes the trampoline addresses into these during InstallAll().
    // Call these from inside the detours to invoke the real RVGL functions.
    // ------------------------------------------------------------------------
    extern FnDrawPostRaceLeaderboard Orig_DrawPostRaceLeaderboard;
    extern FnRaceSessionSetup        Orig_RaceSessionSetup;
    extern FnAssignStartPositions    Orig_AssignStartPositions;
    extern FnSetupAllRaceCars        Orig_SetupAllRaceCars;
    extern FnRandomizeCarPicks       Orig_RandomizeCarPicks;
    extern FnAddParticipantAndCount  Orig_AddParticipantAndCount;
    extern FnUpdateRacePositions     Orig_UpdateRacePositions;

    // ------------------------------------------------------------------------
    // Detour functions — registered in HookManager.cpp → RegisterHooks().
    // ------------------------------------------------------------------------
    void Hook_DrawPostRaceLeaderboard();
    void Hook_RaceSessionSetup(bool isRestart);
    void Hook_AssignStartPositions();
    void Hook_SetupAllRaceCars();
    void Hook_RandomizeCarPicks();
    bool Hook_AddParticipantAndCount(int carType, int spawnType, int carID, int skinID, int isLocal, int networkID, char *playerName);
    void Hook_UpdateRacePositions();

} // namespace Randomizer
