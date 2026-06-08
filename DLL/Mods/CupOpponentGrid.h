#pragma once
#include "ExtendedCupResults.h"
#include "ConfigData.h"
#include "RVGLStructs.h"

namespace Randomizer {

    // ------------------------------------------------------------------------
    // Function pointer types — must exactly match the RVGL originals.
    // ------------------------------------------------------------------------
    using FnCup_GenerateOpponentGrid = void(*)();

    // ------------------------------------------------------------------------
    // Original function pointers.
    // MinHook writes the trampoline addresses into these during InstallAll().
    // Call these from inside the detours to invoke the real RVGL functions.
    // ------------------------------------------------------------------------
    extern FnCup_GenerateOpponentGrid Orig_Cup_GenerateOpponentGrid;

    // ------------------------------------------------------------------------
    // Detour functions — registered in HookManager.cpp → RegisterHooks().
    // ------------------------------------------------------------------------
    void Hook_Cup_GenerateOpponentGrid();

    bool IsExtendedCupOpponentGrid(CupProfile* cup);

} // namespace Randomizer
