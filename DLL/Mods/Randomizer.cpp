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
FnLoadTextureByName   Orig_LoadTextureByName   = nullptr;

// ----------------------------------------------------------------------------
// Car pool snapshot
// Populated in Hook_LoadCars once the pool is built.
// ----------------------------------------------------------------------------
static std::vector<CarInfo> s_carPool;
static int                  s_carCount = 0;

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
    CarInfo* rawPool = *reinterpret_cast<CarInfo**>(AbsFromRva(RVA_CAR_TABLE));
    s_carCount = *reinterpret_cast<int*>(AbsFromRva(RVA_CAR_COUNT));

    // Copy cars to s_carPool vector to keep them in the DLL memory
    s_carPool.clear();
    if (rawPool != nullptr && s_carCount > 0) {
        s_carPool.assign(rawPool, rawPool + s_carCount);
    }

    OutputDebugStringA(("[Randomizer] Car pool snapshot — "
                        + std::to_string(s_carCount) + " cars\n").c_str());

    return result;
}

unsigned long long Hook_LoadTextureByName(char* path, int slotID, int maxMipLevel, bool enableMips, int param_5, unsigned int flags) {

    std::string customAtlasPath = "";
    char* targetPath = path; // Default to the original path
    
    int carboxNumber = GetCarboxNumberFromPath(path);
    
    // If it returned 1 through 5, it's a match
    if (carboxNumber != 0) {
        
        std::vector<CarboxSource> randomGrid = GetGridSourcesForCarbox(carboxNumber, s_carPool.data());

        // GetGridSourcesForCarbox returns empty if there are no changes
        if (!randomGrid.empty()) {
            
            // Generate the stitched file to disk
            customAtlasPath = "cars/misc/carbox_random_" + std::to_string(carboxNumber) + ".bmp";
            GenerateAndSaveCarboxAtlas(customAtlasPath, randomGrid);

            // Point our targetPath to the newly generated file instead of overwriting memory
            targetPath = const_cast<char*>(customAtlasPath.c_str());
        }
    }

    Logger::TimestampLogf("[LoadTextureByName] Calling on path %s for slot ID %d", targetPath, slotID);
                
    // Call the original texture loader
    unsigned long long returnValue = Orig_LoadTextureByName(targetPath, slotID, maxMipLevel, enableMips, param_5, flags);

    if (customAtlasPath != "") {
        // delete random carbox bmp
        std::remove(customAtlasPath.c_str());
    }
    return returnValue;
}


} // namespace Randomizer
