#include "HookManager.h"
#include "MinHook.h"
#include "Addresses.h"
#include "Randomizer.h"   // declares Randomizer::Hook_LoadCars, Hook_BuildGrid, etc.
#include "CallLogger.h"
#include "Logger.h"
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
// HookManager::Add  (typed hooks — when you need to inspect args or modify
//                    behaviour, not just log the call)
// ============================================================================

bool HookManager::Add(uintptr_t target, void* detour, void** original, const char* name) {
    if (!target) {
        // Address resolved to zero — RVA is wrong or module base not found.
        Logger::TimestampLogf("[HookManager] SKIP (zero address): %s", name);
        return false;
    }

    MH_STATUS status = MH_CreateHook(reinterpret_cast<void*>(target), detour, original);

    if (status != MH_OK && status != MH_ERROR_ALREADY_CREATED) {
        Logger::TimestampLogf("[HookManager] FAIL MH_CreateHook: %s — %s",
                     name, MH_StatusToString(status));
        return false;
    }

    status = MH_EnableHook(reinterpret_cast<void*>(target));

    if (status != MH_OK && status != MH_ERROR_ENABLED) {
        Logger::TimestampLogf("[HookManager] FAIL MH_EnableHook: %s — %s",
                     name, MH_StatusToString(status));
        return false;
    }

    g_hooks.push_back({ target, detour, original, name, true });
    Logger::TimestampLogf("[HookManager] OK: %s", name);
    return true;
}

// ============================================================================
// Hook list
//
// This is the only place that knows which RVGL functions are intercepted.
// Two categories:
//
//  1. TYPED HOOKS (HookManager::Add)
//     Use these when you need to inspect or modify arguments/return values.
//     Each one requires a Detour + Original pair in a Mods/ file.
//
//     Each entry follows the pattern:
//
//       Add(AbsFromRva(RVA_FUNCTION), Detour, &Original, "DisplayName");
//
//       AbsFromRva  — converts the compile-time RVA constant to a runtime address
//       Detour      — your replacement function defined in e.g. Randomizer.cpp
//       &Original   — trampoline pointer; call this inside the detour to call through
//       DisplayName — shown in debug output to identify which hook failed
//
//     Adding a new hook to the mod means adding one line here and defining the
//     detour/original pair in the relevant file under Mods/. Nothing else changes.
//
//  2. AUTO LOG HOOKS (CallLogger::RegisterAll)
//     Use these when you just want to know that a function was called.
//     Pass a map of { RVA → display name }. No extra code needed.
//     See CallLogger.h for the float-param caveat.
// ============================================================================

static void RegisterHooks() {

    // ------------------------------------------------------------------
    // 1. Typed hooks — mod logic lives in the corresponding Mods/ file
    // ------------------------------------------------------------------

    // --- Car system ---
    HookManager::Add(
        AbsFromRva(RVA_LOAD_VANILLA_CAR_POOL),
        reinterpret_cast<void*>(Randomizer::Hook_LoadVanillaCarPool),
        reinterpret_cast<void**>(&Randomizer::Orig_LoadVanillaCarPool),
        "LoadVanillaCarPool"
    );

    HookManager::Add(
        AbsFromRva(RVA_LOAD_CUSTOM_CAR_POOL),
        reinterpret_cast<void*>(Randomizer::Hook_LoadCustomCarPool),
        reinterpret_cast<void**>(&Randomizer::Orig_LoadCustomCarPool),
        "LoadCustomCarPool"
    );

    HookManager::Add(
        AbsFromRva(RVA_SYNC_CAR_INFO_FROM_PHYSICS),
        reinterpret_cast<void*>(Randomizer::Hook_SyncCarInfoFromPhysics),
        reinterpret_cast<void**>(&Randomizer::Orig_SyncCarInfoFromPhysics),
        "SyncCarInfoFromPhysics"
    );

    HookManager::Add(
        AbsFromRva(RVA_LOAD_TEXTURE_BY_NAME),
        reinterpret_cast<void*>(Randomizer::Hook_LoadTextureByName),
        reinterpret_cast<void**>(&Randomizer::Orig_LoadTextureByName),
        "LoadTextureByName"
    );

    HookManager::Add(
        AbsFromRva(RVA_LOAD_VANILLA_TRACKS),
        reinterpret_cast<void*>(Randomizer::Hook_LoadVanillaTracks),
        reinterpret_cast<void**>(&Randomizer::Orig_LoadVanillaTracks),
        "LoadVanillaTracks"
    );

    HookManager::Add(
        AbsFromRva(RVA_LOAD_CUSTOM_TRACKS),
        reinterpret_cast<void*>(Randomizer::Hook_LoadCustomTracks),
        reinterpret_cast<void**>(&Randomizer::Orig_LoadCustomTracks),
        "LoadCustomTracks"
    );

    
    // Add further hooks here as the mod grows, e.g.:
    //
    // HookManager::Add(
    //     AbsFromRva(RVA_CALC_DELTA_TIME),
    //     reinterpret_cast<void*>(SlowMo::Hook_CalcDeltaTime),
    //     reinterpret_cast<void**>(&SlowMo::Orig_CalcDeltaTime),
    //     "CalcDeltaTime"
    // );

    // ------------------------------------------------------------------
    // 2. Auto log hooks — just a map, no extra code required
    //
    // Every function listed here will print its name to the log and the
    // VS Code Debug Console each time it is called by RVGL, then
    // transparently call the real function.
    //
    // RVAs from Addresses.h or the knowledge base (subtract image base
    // 0x00400000 from the Ghidra address to get the RVA).
    // ------------------------------------------------------------------

    CallLogger::RegisterAll({
        // Game loop
        // { 0x00001610, "FrameUpdateDispatcher"       },

        // Car system
        // { 0x0003F140, "LoadVanillaCarPool"          },   // same as typed hook above — pick one or the other
        // { 0x0003FAC0, "LoadCustomCarPool"           },
        { 0x000F03A0, "CreateCarEntity"             },
        { 0x000F0630, "DestroyCarEntity"            },
        { 0x0004CB60, "InitPlayerCar"               },

        // Race session
        // { 0x00050700, "RaceSessionSetup"             },
        // { 0x0004F7B0, "SetupAllRaceCars"             },
        // { 0x0004FB30, "RandomizeCarPicks"            },
        // { 0x0004F980, "AssignStartPositions"         },
        // { 0x00041F80, "RaceEndManager"               }, // NOTE: check RVA
        // { 0x0008A560, "SetRaceState"                 },

        // Track / navigation
        // { 0x00052190, "LoadAllTracks"                },
        // { 0x00051AB0, "SetupRaceTrackRandom"         },
        // { 0x00040C30, "LevelLoad"                    },

        // Ghost / replay
        // { 0x0004CEC0, "RecordGhostFrame"             },
        // { 0x0004D040, "CommitGhostLapFrame"          },
        // { 0x0004D0C0, "RecordGhostPosition"          },

        // Carboxes
        { 0x00143890, "Menu_InitializeFrontend" },
        { 0x001424a0, "CarSelector_UpdateAnimationAndDraw" },
        { 0x00141f80 , "RaceEndManager" }
    });
}

// ============================================================================
// HookManager::InstallAll / RemoveAll
// ============================================================================

bool HookManager::InstallAll() {
    // Initialise MinHook once for the lifetime of the DLL.
    MH_STATUS status = MH_Initialize();

    if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED) {
        Logger::TimestampLogf("[HookManager] FAIL MH_Initialize — %s",
                     MH_StatusToString(status));
        return false;
    }

    g_minHookInitialized = true;
    RegisterHooks();

    // Report overall result but don't treat individual hook failures as fatal —
    // a partially-working mod is more useful than a mod that refuses to run.
    const size_t installed = g_hooks.size();
    Logger::TimestampLogf("[HookManager] InstallAll complete — %zu typed hook(s) active",
                 installed);

    return installed > 0;
}

void HookManager::RemoveAll() {
    if (!g_minHookInitialized)
        return;

    // Disable every typed hook we installed before uninitializing MinHook.
    for (const auto& entry : g_hooks) {
        if (entry.installed)
            MH_DisableHook(reinterpret_cast<void*>(entry.target));
    }

    g_hooks.clear();

    // Disable auto-log hooks
    CallLogger::UnregisterAll();
    
    MH_Uninitialize();
    g_minHookInitialized = false;

    Logger::TimestampLog("[HookManager] RemoveAll complete");
}
