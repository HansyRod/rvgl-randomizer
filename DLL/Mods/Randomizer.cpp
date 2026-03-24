#include "Randomizer.h"
#include "Addresses.h"
#include "RVGLStructs.h"
#include "Carbox.h"
#include "GameUtils.h"
#include "Image.h"
#include "Logger.h"
#include <windows.h>
#include <string>
#include <cstdint>
#include <vector>

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
FnLoadVanillaCarPool     Orig_LoadVanillaCarPool     = nullptr;
FnLoadTextureByName      Orig_LoadTextureByName      = nullptr;
FnLoadCustomCarPool      Orig_LoadCustomCarPool      = nullptr;
FnSyncCarInfoFromPhysics Orig_SyncCarInfoFromPhysics = nullptr;

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

    std::string customTexturePath = "";
    char* targetPath = path; // Default to the original path
    
    // --- SLOT 142: SINGLE CUSTOM CARBOX ---
    if (slotID == 142) {
        std::string pathStr(path);
        const std::string prefix = "cars/misc/custom_carbox_";
        size_t prefixPos = pathStr.find(prefix);
        
        // Is this one of our injected dummy paths?
        if (prefixPos != std::string::npos) {
            size_t startIdx = prefixPos + prefix.length();
            size_t endIdx = pathStr.find(".bmp", startIdx);
            
            if (endIdx != std::string::npos) {
                std::string internalName = pathStr.substr(startIdx, endIdx - startIdx);
                
                // Fetch the source (which will route back to its vanilla 84x84 cell)
                CarboxSource src = GetVanillaCarboxSource(internalName);
                
                customTexturePath = pathStr; // Keep the same path for the temp file
                GenerateAndSaveSingleCarbox(customTexturePath, src);
                
                targetPath = const_cast<char*>(customTexturePath.c_str());
            }
        }
    }
    // --- SLOTS 143-147: VANILLA GRID CARBOXES ---
    else if (slotID >= 143 && slotID <= 147) {
        int carboxNumber = GetCarboxNumberFromPath(path);
        
        // If it returned 1 through 5, it's a match
        if (carboxNumber != 0) {
            
            std::vector<CarboxSource> randomGrid = GetGridSourcesForCarbox(carboxNumber, s_carPool.data());

            // GetGridSourcesForCarbox returns empty if there are no changes
            if (!randomGrid.empty()) {
                
                // Generate the stitched file to disk
                customTexturePath = "cars/misc/carbox_random_" + std::to_string(carboxNumber) + ".bmp";
                GenerateAndSaveCarboxAtlas(customTexturePath, randomGrid);

                // Point our targetPath to the newly generated file instead of overwriting memory
                targetPath = const_cast<char*>(customTexturePath.c_str());
            }
        }
    }

    Logger::TimestampLogf("[LoadTextureByName] Calling on path %s for slot ID %d", targetPath, slotID);
                
    // Call the original texture loader
    unsigned long long returnValue = Orig_LoadTextureByName(targetPath, slotID, maxMipLevel, enableMips, param_5, flags);

    if (customTexturePath != "") {
        // delete random carbox bmp
        std::remove(customTexturePath.c_str());
    }
    return returnValue;
}


void Hook_LoadCustomCarPool() {
    // Let RVGL load the custom cars from disk into memory first
    Orig_LoadCustomCarPool();

    CarInfo* customPool = *reinterpret_cast<CarInfo**>(AbsFromRva(RVA_CAR_TABLE));
    int customCount     = *reinterpret_cast<int*>(AbsFromRva(RVA_CAR_COUNT));

    // Check if any cars were added to the car table
    if (customPool != nullptr && customCount > s_carCount) {
        // Iterate only over the custom cars
        for (int i = s_carCount; i < customCount; ++i) {
            CarInfo* car = &customPool[i];
            
            ApplyCarMods(i, car, nullptr);

            // Append to our persistent snapshot so GetGridSourcesForCarbox can find it
            s_carPool.push_back(*car); 
        }
    }

    return;
}


void Hook_SyncCarInfoFromPhysics(int carIndex, CarPhysicsData *physData) {

    if (carIndex >= 49) {
        CarInfo* carPool = s_carPool.data();
        CarInfo* car = &carPool[carIndex];

        ApplyCarMods(carIndex, car, physData);
    }

    Orig_SyncCarInfoFromPhysics(carIndex, physData);

}

void ApplyCarMods(int carIndex, CarInfo* car, CarPhysicsData *physData) {

    // Apply these only to cars in custom pool
    if (carIndex >= 49) {
        // If the car lacks a TCARBOX property
        if (car->tcarboxFilename[0] == '\0' || (physData != nullptr && physData->tcarboxFilename[0] == '\0')) {
            // Generate a unique dummy path we can intercept later
            std::string dummyPath = "cars/misc/custom_carbox_" + std::string(car->internalName) + ".bmp";
            strncpy_s(car->tcarboxFilename, 64, dummyPath.c_str(), _TRUNCATE);
            if (physData != nullptr) {
                strncpy_s(physData->tcarboxFilename, 64, dummyPath.c_str(), _TRUNCATE);
            }
        }

        // If the car is stock/dc and has statistics disabled, enable them
        if (IsStockCar(car->internalName)) {
            car->statisticsEnabled = true;
            if (physData != nullptr) {
                physData->statistics = true;
            }
        }
    }
}

} // namespace Randomizer
