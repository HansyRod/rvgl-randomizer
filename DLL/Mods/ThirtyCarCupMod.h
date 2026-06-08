#pragma once
#include <cstdint>
#include <cstdio>
#include "RVGLStructs.h"
#include "ExtendedCupResults.h"
#include "ConfigData.h"

namespace Randomizer {

    // ------------------------------------------------------------------------
    // Function pointer types — must exactly match the RVGL originals.
    // ------------------------------------------------------------------------
    using FnBuildGrid = void(*)();
    using FnUpdateCupPostRaceProgress = void(*)();
    using FnDrawCupStandingsTable = void(*)();

    // ------------------------------------------------------------------------
    // Original function pointers.
    // MinHook writes the trampoline addresses into these during InstallAll().
    // Call these from inside the detours to invoke the real RVGL functions.
    // ------------------------------------------------------------------------
    extern FnBuildGrid Orig_BuildGrid;
    extern FnUpdateCupPostRaceProgress Orig_UpdateCupPostRaceProgress;
    extern FnDrawCupStandingsTable Orig_DrawCupStandingsTable;

    // ------------------------------------------------------------------------
    // Detour functions — registered in HookManager.cpp → RegisterHooks().
    // ------------------------------------------------------------------------
    void Hook_BuildGrid();
    void Hook_UpdateCupPostRaceProgress();
    void Hook_DrawCupStandingsTable();

    bool IsThirtyCarCupActive();
    void ResetThirtyCarCupState();
    void StartThirtyCarCupState(
        int selectedCupIndex,
        CupProfile* cup,
        const RandomizedCup* cupConfig,
        const ExtendedCupResultsState& results
    );
    void ApplyThirtyCarCupGrid();
    void MoveThirtyCarCupPlayerToBackAfterRacePositions();
    bool HandleThirtyCarCupOnStageFinished();

} // namespace Randomizer
