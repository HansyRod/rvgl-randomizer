#pragma once

namespace Randomizer {

    // ------------------------------------------------------------------------
    // Function pointer types — must exactly match the RVGL originals.
    // Ghidra signatures (x64, __fastcall is noise and is dropped):
    //   LoadVanillaCarPool:  bool ()
    // ------------------------------------------------------------------------
    using FnDrawNumericMenuValue        = void(*)(int panelIndex, int itemIndex, void* unused, void* renderContext);
    using FnDecrementNumericMenuValue   = bool(*)(int panelIndex);
    using FnIncrementNumericMenuValue   = bool(*)(int panelIndex);

    // ------------------------------------------------------------------------
    // Original function pointers.
    // Declared here, defined in Randomizer.cpp.
    // MinHook writes the trampoline addresses into these during InstallAll().
    // Call these from inside the detours to invoke the real RVGL functions.
    // ------------------------------------------------------------------------
    extern FnDrawNumericMenuValue       Orig_DrawNumericMenuValue;
    extern FnDecrementNumericMenuValue  Orig_DecrementNumericMenuValue;
    extern FnIncrementNumericMenuValue  Orig_IncrementNumericMenuValue;

    // ------------------------------------------------------------------------
    // Detour functions — registered in HookManager.cpp → RegisterHooks().
    // ------------------------------------------------------------------------
    // void Hook_LoadObjectsFromFob(char* fobFilePath);

    // Other functions exposed by this mod
    bool IncrementRandomizerCarCount(int panelIndex);
    bool DecrementRandomizerCarCount(int panelIndex);
    void PatchCarCountMenuDescriptor();

} // namespace Randomizer
