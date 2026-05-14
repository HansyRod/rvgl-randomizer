#pragma once

namespace Randomizer {

    // ------------------------------------------------------------------------
    // Function pointer types — must exactly match the RVGL originals.
    // Ghidra signatures (x64, __fastcall is noise and is dropped):
    //   LoadVanillaCarPool:  bool ()
    // ------------------------------------------------------------------------
    using FnLoadObjectsFromFob      = void(*)(char* fobFilePath);
    using FnCreateObjectFromFob     = void*(*)(float* position, float* rotationMatrix, unsigned int objectId, int* subinfos);

    // ------------------------------------------------------------------------
    // Original function pointers.
    // Declared here, defined in Randomizer.cpp.
    // MinHook writes the trampoline addresses into these during InstallAll().
    // Call these from inside the detours to invoke the real RVGL functions.
    // ------------------------------------------------------------------------
    extern FnLoadObjectsFromFob     Orig_LoadObjectsFromFob;
    extern FnCreateObjectFromFob    Orig_CreateObjectFromFob;

    // ------------------------------------------------------------------------
    // Detour functions — registered in HookManager.cpp → RegisterHooks().
    // ------------------------------------------------------------------------
    void Hook_LoadObjectsFromFob(char* fobFilePath);

} // namespace Randomizer
