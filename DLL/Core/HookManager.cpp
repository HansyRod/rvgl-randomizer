#include "HookManager.h"
#include "MinHook.h"
#include "Addresses.h"
#include "Randomizer.h"   // declares Randomizer::Hook_LoadCars, Hook_BuildGrid, etc.
#include <string>
#include <vector>

// ============================================================================
// Internal state
// ============================================================================

namespace {

struct HookEntry {
    uintptr_t   target;
    void*       detour;
    void**      original;
    const char* name;
    bool        installed;
};

std::vector<HookEntry> g_hooks;
bool                   g_minHookInitialized = false;

} // anonymous namespace

// ============================================================================
// HookManager::Add
// ============================================================================

bool HookManager::Add(uintptr_t target, void* detour, void** original, const char* name) {
    if (!target) {
        // Address resolved to zero — RVA is wrong or module base not found.
        OutputDebugStringA(("[HookManager] SKIP (zero address): " + std::string(name) + "\n").c_str());
        return false;
    }

    MH_STATUS status = MH_CreateHook(reinterpret_cast<void*>(target), detour, original);

    if (status != MH_OK && status != MH_ERROR_ALREADY_CREATED) {
        OutputDebugStringA(("[HookManager] FAIL MH_CreateHook: " + std::string(name)
                           + " — " + MH_StatusToString(status) + "\n").c_str());
        return false;
    }

    status = MH_EnableHook(reinterpret_cast<void*>(target));

    if (status != MH_OK && status != MH_ERROR_ENABLED) {
        OutputDebugStringA(("[HookManager] FAIL MH_EnableHook: " + std::string(name)
                           + " — " + MH_StatusToString(status) + "\n").c_str());
        return false;
    }

    g_hooks.push_back({ target, detour, original, name, true });
    OutputDebugStringA(("[HookManager] OK: " + std::string(name) + "\n").c_str());
    return true;
}

// ============================================================================
// Hook list
//
// This is the only place that knows which RVGL functions are intercepted.
// Each entry follows the pattern:
//
//   Add(AbsFromRva(RVA_FUNCTION), Detour, &Original, "DisplayName");
//
//   AbsFromRva  — converts the compile-time RVA constant to a runtime address
//   Detour      — your replacement function defined in e.g. Randomizer.cpp
//   &Original   — trampoline pointer; call this inside the detour to call through
//   DisplayName — shown in debug output to identify which hook failed
//
// Adding a new hook to the mod means adding one line here and defining the
// detour/original pair in the relevant file under Mods/. Nothing else changes.
// ============================================================================

static void RegisterHooks() {
    // --- Car system ---
    HookManager::Add(
        AbsFromRva(RVA_LOAD_VANILLA_CAR_POOL),
        reinterpret_cast<void*>(Randomizer::Hook_LoadVanillaCarPool),
        reinterpret_cast<void**>(&Randomizer::Orig_LoadVanillaCarPool),
        "LoadAllCars"
    );

    // Add further hooks here as the mod grows, e.g.:
    //
    // HookManager::Add(
    //     AbsFromRva(RVA_CALC_DELTA_TIME),
    //     reinterpret_cast<void*>(SlowMo::Hook_CalcDeltaTime),
    //     reinterpret_cast<void**>(&SlowMo::Orig_CalcDeltaTime),
    //     "CalcDeltaTime"
    // );
}

// ============================================================================
// HookManager::InstallAll / RemoveAll
// ============================================================================

bool HookManager::InstallAll() {
    // Initialise MinHook once for the lifetime of the DLL.
    MH_STATUS status = MH_Initialize();

    if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED) {
        OutputDebugStringA(("[HookManager] FAIL MH_Initialize — "
                            + std::string(MH_StatusToString(status)) + "\n").c_str());
        return false;
    }

    g_minHookInitialized = true;
    RegisterHooks();

    // Report overall result but don't treat individual hook failures as fatal —
    // a partially-working mod is more useful than a mod that refuses to run.
    const size_t installed = g_hooks.size();
    OutputDebugStringA(("[HookManager] InstallAll complete — "
                        + std::to_string(installed) + " hook(s) active\n").c_str());

    return installed > 0;
}

void HookManager::RemoveAll() {
    if (!g_minHookInitialized)
        return;

    // Disable every hook we installed before uninitializing MinHook.
    for (const auto& entry : g_hooks) {
        if (entry.installed)
            MH_DisableHook(reinterpret_cast<void*>(entry.target));
    }

    g_hooks.clear();
    MH_Uninitialize();
    g_minHookInitialized = false;

    OutputDebugStringA("[HookManager] RemoveAll complete\n");
}
