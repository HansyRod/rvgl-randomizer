#pragma once
#include <cstdint>
#include <string>
#include <windows.h>

// ============================================================================
// HookManager
//
// Single point of entry for all function hooks in the mod.
// DllMain calls HookManager::InstallAll() on attach and
// HookManager::RemoveAll() on detach.
//
// To add a new hook:
//   1. Define your detour and original function pointer in your mod file
//      under Mods/ (e.g. Randomizer.cpp).
//   2. Add one HookManager::Add() call in RegisterHooks() in HookManager.cpp.
//   Nothing else needs to change.
// ============================================================================

namespace HookManager {

    // -------------------------------------------------------------------------
    // Install / remove all hooks defined in InstallAll().
    // Called by DllMain — do not call these from anywhere else.
    // -------------------------------------------------------------------------
    bool InstallAll();
    void RemoveAll();

    // -------------------------------------------------------------------------
    // Hook a single function by absolute address.
    //
    //   target   — absolute address of the function to hook (already resolved
    //              from RVA via AbsFromRva)
    //   detour   — your replacement function (must match the calling convention
    //              and signature of the original)
    //   original — receives a pointer to the trampoline so you can call through
    //              to the original function from inside your detour
    //   name     — label used only in debug output
    //
    // Returns false and logs a reason on failure. Failures are non-fatal —
    // InstallAll() continues installing remaining hooks.
    // -------------------------------------------------------------------------
    bool Add(uintptr_t target, void* detour, void** original, const char* name);

} // namespace HookManager
