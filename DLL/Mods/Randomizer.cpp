#include "Randomizer.h"
#include "Addresses.h"
#include "RVGLStructs.h"
#include <windows.h>
#include <string>
#include <cstdint>

// ============================================================================
// Randomizer.cpp
//
// Mod logic for randomizing the car grid.
// This file is responsible only for what the hooks do — not how they are
// installed. See HookManager.cpp for installation.
// ============================================================================

namespace Randomizer {

// ----------------------------------------------------------------------------
// Original function pointers
// MinHook writes the trampoline addresses into these during InstallAll().
// ----------------------------------------------------------------------------
FnLoadVanillaCarPool  Orig_LoadVanillaCarPool  = nullptr;

// ----------------------------------------------------------------------------
// Car pool snapshot
// Populated in Hook_LoadCars once the pool is built.
// ----------------------------------------------------------------------------
static CarInfo* s_carPool  = nullptr;
static int      s_carCount = 0;

// ============================================================================
// Hook_LoadCars
//
// Fires after RVGL has loaded all cars into the pool.
// We call through to the original first so the pool is fully built,
// then snapshot the pool pointer and count for use in Hook_BuildGrid.
// ============================================================================
bool Hook_LoadVanillaCarPool() {
    // Call the real LoadVanillaCarPool so RVGL loads its cars normally.
    const bool result = Orig_LoadVanillaCarPool();

    // Read the pool pointer and count that RVGL just populated.
    // AbsFromRva resolves the RVA to the actual runtime address, then we
    // dereference it to get the value stored at that address.
    s_carPool  = *reinterpret_cast<CarInfo**>(AbsFromRva(RVA_CAR_TABLE));
    s_carCount = *reinterpret_cast<int*>(AbsFromRva(RVA_CAR_COUNT));

    OutputDebugStringA(("[Randomizer] Car pool snapshot — "
                        + std::to_string(s_carCount) + " cars\n").c_str());

    return result;
}

} // namespace Randomizer
