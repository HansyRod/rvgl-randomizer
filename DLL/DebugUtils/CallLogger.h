#pragma once
#include <cstdint>
#include <unordered_map>

// ============================================================================
// CallLogger
//
// Registers passthrough logging hooks in bulk from a map of:
//   { RVA (uint32_t)  →  display name (const char*) }
//
// Each hook logs the function name when called, then calls through to the
// original, preserving all arguments and return values transparently.
//
// IMPORTANT — Signature limitation
// ---------------------------------
// The generic detour uses the signature:
//   void* __fastcall (void*, void*, void*, void*)
// This works correctly for any RVGL function whose first four arguments are
// integers or pointers (passed in RCX, RDX, R8, R9).
//
// It does NOT correctly pass through functions whose arguments include floats
// or XMM registers (e.g. CalcDeltaTime, physics solvers). For those, write
// a properly typed hook in HookManager.cpp instead.
//
// Capacity
// --------
// The pool is compile-time fixed at MAX_AUTO_LOG_HOOKS entries (default 64).
// Increase the constant if you need more.
//
// Usage
// -----
//   // In HookManager.cpp (or wherever you call HookManager::InstallAll):
//
//   CallLogger::RegisterAll({
//       { RVA_LOAD_CARS,              "LoadAllCars"            },
//       { RVA_RACE_SESSION_SETUP,     "RaceSessionSetup"       },
//       { RVA_INIT_PLAYER_CAR,        "InitPlayerCar"          },
//       { RVA_RENDER_SCENE_FULL,      "RenderSceneFull"        },
//       { RVA_PER_FRAME_PHYSICS_STEP, "PerFramePhysicsStep"    },
//   });
//
//   // Then in your mod logic hooks you can still use the typed hooks as normal.
//   // CallLogger hooks and typed hooks can coexist on different functions.
// ============================================================================

namespace CallLogger {

    // Maximum number of auto-log hooks that can be registered in one session.
    // This is a compile-time constant — the hook slots are pre-instantiated.
    static constexpr int MAX_AUTO_LOG_HOOKS = 64;

    // Register a batch of passthrough logging hooks.
    // Call this from HookManager::RegisterHooks() after MH_Initialize().
    // Returns the number of hooks successfully installed.
    int RegisterAll(const std::unordered_map<uint32_t, const char*>& functions);

    // Disable and remove all hooks installed by RegisterAll().
    // Called from HookManager::RemoveAll().
    void UnregisterAll();

} // namespace CallLogger