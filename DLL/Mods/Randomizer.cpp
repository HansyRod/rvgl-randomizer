#include "Randomizer.h"
#include "Addresses.h"
#include "RVGLStructs.h"
#include "Carbox.h"
#include "GameUtils.h"
#include "ConfigManager.h"
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

namespace {

// ============================================================================
// Hardcoded car list for testing
// Replace entries with whichever car folders you want to load.
// Must be exactly 49 entries to fill the vanilla pool.
// Every string must follow the "cars/<foldername>" format.
// ============================================================================
static const char* k_TestCarPaths[49] = {
    "cars/rc",
    "cars/albatrossgt",
    "cars/phat",
    "cars/rotor",
    "cars/mud",
    "cars/beatall",
    "cars/volken",
    "cars/tc6",
    "cars/fd64lost",
    "cars/candy",
    "cars/gencar",
    "cars/tc4",
    "cars/mouse",
    "cars/flag",
    "cars/tc2",
    "cars/r5",
    "cars/tc5",
    "cars/sgt",
    "cars/tc3",
    "cars/adeon",
    "cars/fone",
    "cars/tc1",
    "cars/moss",
    "cars/cougar",
    "cars/sugo",
    "cars/toyeca",
    "cars/amw",
    "cars/panga",
    "cars/trolley",
    "cars/wincar",
    "cars/wincar2",
    "cars/wincar3",
    "cars/wincar4",
    "cars/ufo",
    "cars/q",
    "cars/bigvolt",
    "cars/bossvolt",
    "cars/jg6rc",
    "cars/tc12",
    "cars/tc10",
    "cars/tc8",
    "cars/tc11",
    "cars/tc9",
    "cars/jg1jg7",
    "cars/tc7",
    "cars/jg3loco",
    "cars/jg4snw35",
    "cars/jg5purpxl",
    "cars/jg2fulonx"
};

static const char* k_TestTrackNames[14] = {
    "venice",
    "roof",
    "ship1",
    "toylite",
    "garden1",
    "nhood2",
    "toy2",
    "market1",
    "wild_west2",
    "muse1",
    "muse2",
    "ship2",
    "nhood1",
    "wild_west1"
};

// Persistent storage for the patched pointer table.
// The pointers in g_VanillaCarPaths must remain valid after the hook returns,
// so these are static rather than stack-allocated.
static const char* s_patchedPtrs[49];

void InitHardcodedCarPaths() {
    for (int i = 0; i < 49; i++)
        s_patchedPtrs[i] = k_TestCarPaths[i];
}

} // anonymous namespace

namespace Randomizer {

// Global or class-member variable to hold the active configuration
std::optional<ConfigData> g_ActiveConfig = std::nullopt;

void Initialize() {
    Logger::TimestampLogf("[Randomizer] Initializing Randomizer module...");

    // 1. Attempt to load the configuration file
    // Make sure the path matches where you place the test JSON relative to rvgl.exe
    const std::string configPath = "randomizer_config.json"; 
    
    g_ActiveConfig = LoadConfiguration(configPath);

    // 2. Verify successful load and output test data
    if (g_ActiveConfig.has_value()) {
        Logger::TimestampLogf("[Randomizer] Successfully loaded configuration!");
        Logger::TimestampLogf("[Randomizer] Seed: %s", g_ActiveConfig->metadata.seed.c_str());
        Logger::TimestampLogf("[Randomizer] Parsed %zu cars and %zu tracks.", 
            g_ActiveConfig->cars.size(), 
            g_ActiveConfig->tracks.size());
        
        // Example of accessing nested data
        if (!g_ActiveConfig->cars.empty()) {
            Logger::TimestampLogf("[Randomizer] First car folder: %s (Rating: %d)", 
                g_ActiveConfig->cars[0].folder.c_str(), 
                g_ActiveConfig->cars[0].rating);
            
            // Init s_patchedPtrs based on config
            for (size_t i = 0; i < g_ActiveConfig->cars.size() && i < 49; ++i) {
                std::string carPath = g_ActiveConfig->cars[i].folder;
                if (!carPath._Starts_with("cars/")) {
                    carPath = "cars/" + carPath; // Ensure the path has the correct prefix
                }
                g_ActiveConfig->cars[i].folder = carPath; // Update the folder in the config struct
                s_patchedPtrs[i] = g_ActiveConfig->cars[i].folder.c_str();
            }
        }
    } else {
        Logger::TimestampLogf("[Randomizer] Failed to load or parse %s. Mod will remain inactive.", configPath.c_str());
        // Depending on architecture, you might want to disable further hooks here
    }
}

// ----------------------------------------------------------------------------
// Original function pointers
// MinHook writes the trampoline addresses into these during InstallAll().
// ----------------------------------------------------------------------------
FnLoadVanillaCarPool     Orig_LoadVanillaCarPool     = nullptr;
FnLoadTextureByName      Orig_LoadTextureByName      = nullptr;
FnLoadCustomCarPool      Orig_LoadCustomCarPool      = nullptr;
FnSyncCarInfoFromPhysics Orig_SyncCarInfoFromPhysics = nullptr;
FnLoadVanillaTracks      Orig_LoadVanillaTracks      = nullptr;
FnLoadCustomTracks       Orig_LoadCustomTracks       = nullptr;
FnLoadVanillaCups        Orig_LoadVanillaCups        = nullptr;
FnLoadCustomCups         Orig_LoadCustomCups         = nullptr;
FnUpdateCarSelectability Orig_UpdateCarSelectability = nullptr;

// ----------------------------------------------------------------------------
// Car pool snapshot
// Populated in Hook_LoadCars once the pool is built.
// ----------------------------------------------------------------------------
static std::vector<CarInfo> s_carPool;
static int                  s_carCount = 0;

// ----------------------------------------------------------------------------
// Track pool snapshot
// 
// ----------------------------------------------------------------------------
static TrackInfo trackInfoBackup[14] = {};
static std::vector<TrackInfo> s_vanillaTrackPool;
static std::vector<TrackInfo> s_customTrackPool;
static int s_trackCount = 0;


// ============================================================================
// Hook_LoadCars
//
// Fires after RVGL has loaded all cars into the pool.
// We call through to the original first so the pool is fully built,
// then snapshot the pool pointer and count for use in Hook_BuildGrid.
// ============================================================================
bool Hook_LoadVanillaCarPool() {

    size_t initialCarCount = 49; // Vanilla RVGL has 49 cars in the pool, so we expect this many to be loaded before our mods apply.
    if (g_ActiveConfig.has_value() && !g_ActiveConfig->cars.empty()) {
        initialCarCount = g_ActiveConfig->cars.size();
        Logger::TimestampLogf("[Randomizer] Expecting %d cars based on config.", initialCarCount);
    } else {
        Logger::TimestampLogf("[Randomizer] No cars specified in config, defaulting to vanilla count of 49.");
        InitHardcodedCarPaths();
    }

    const uintptr_t tableAddr = AbsFromRva(RVA_VANILLA_CAR_PATHS);
    const SIZE_T    tableSize  = initialCarCount * sizeof(const char*);
    DWORD old;
    // makes the pages writable, saves the original flags into `old`
    VirtualProtect((void*)tableAddr, tableSize, PAGE_READWRITE, &old);
    // now safe to write
    memcpy((void*)tableAddr, s_patchedPtrs, tableSize);
    // restores the original protection (read-only again)
    VirtualProtect((void*)tableAddr, tableSize, old, &old);

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

    CarInfo* carPool = s_carPool.data();
    for (int i = 0; i < s_carCount; ++i) {
        CarInfo* car = &carPool[i];
        ApplyCarMods(i, car, nullptr);
    }

    Logger::TimestampLogf(("[Randomizer] Car pool snapshot — "
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

    std::string carName = car->internalName;

    // Apply these only to cars in custom pool
    if (carIndex >= 49) {
        // If the car lacks a TCARBOX property
        if (car->tcarboxFilename[0] == '\0' || (physData != nullptr && physData->tcarboxFilename[0] == '\0')) {
            // Generate a unique dummy path we can intercept later
            std::string dummyPath = "cars/misc/custom_carbox_" + std::string(carName) + ".bmp";
            strncpy_s(car->tcarboxFilename, 64, dummyPath.c_str(), _TRUNCATE);
            if (physData != nullptr) {
                strncpy_s(physData->tcarboxFilename, 64, dummyPath.c_str(), _TRUNCATE);
            }
        }

        // If the car is stock/dc and has statistics disabled, enable them
        if (IsStockCar(carName)) {
            car->statisticsEnabled = true;
            if (physData != nullptr) {
                physData->statistics = true;
            }
        }
    }

    if (g_ActiveConfig.has_value()) {
        int carConfigSize = g_ActiveConfig->cars.size();
        if (carIndex < carConfigSize) {
            RandomizedCar& carConfig = g_ActiveConfig->cars[carIndex];
            
            car->rating = carConfig.rating;
            car->obtainCondition = carConfig.obtain;
            // car->selectableByPlayer = carConfig.selectable_player;
            // car->selectableByCPU = carConfig.selectable_cpu;

            if (physData != nullptr) {
                physData->starRating = carConfig.rating;
                physData->obtainCondition = carConfig.obtain;
                // Note: PhysData doesn't have selectable flags, so we won't apply those there
            }
        }
    }

}


void Hook_LoadVanillaTracks() {

    TrackInfo* vanillaTracks = reinterpret_cast<TrackInfo*>(AbsFromRva(RVA_VANILLA_TRACKS_TABLE));
    
    // You can now access it like a standard array
    for (int i = 0; i < 21; i++) {
        TrackInfo* currentTrack = &vanillaTracks[i];

        if (i < 14) {
            // 1. Create a deep copy of the hardcoded data
            trackInfoBackup[i] = *currentTrack;
            // 2. Apply randomization    
            strncpy_s(currentTrack->folderName, 16, k_TestTrackNames[i], _TRUNCATE);
        }
        else {
            Logger::TimestampLogf("[LoadVanillaTracks] Track %d: %s", i+1, currentTrack->displayName);
        }
    }

    Orig_LoadVanillaTracks();

    // Get track count
    s_trackCount = *reinterpret_cast<int*>(AbsFromRva(RVA_TRACK_COUNT));

    // Copy references to vanilla tracks
    s_vanillaTrackPool.clear();
    if (vanillaTracks != nullptr && s_trackCount > 0) {
        s_vanillaTrackPool.assign(vanillaTracks, vanillaTracks + s_trackCount);
    }

    // Apply missing hardcoded data
    for (int i = 0; i < 14; i++) {
        TrackInfo* currentTrack = &vanillaTracks[i];
        Logger::TimestampLogf("[LoadVanillaTracks] Track %d: %s", i+1, currentTrack->displayName);
        ApplyStockTrackData(currentTrack);
    }
}

void Hook_LoadCustomTracks() {
    Orig_LoadCustomTracks();

    TrackInfo* customTracksPool = *reinterpret_cast<TrackInfo**>(AbsFromRva(RVA_CUSTOM_TRACKS_TABLE));
    int trackCount = *reinterpret_cast<int*>(AbsFromRva(RVA_TRACK_COUNT));

    for (int i = 21; i < trackCount; i++) {
        TrackInfo* currentTrack = &customTracksPool[i-21];
        Logger::TimestampLogf("[LoadCustomTracks] Track %d: %s", i+1, currentTrack->displayName);
        ApplyStockTrackData(currentTrack);
    }
}


void ApplyStockTrackData(TrackInfo* track) {

    std::string folderName = track->folderName;
    TrackInfo* backup = nullptr;

    for (int j = 0; j < 14; j++) {
        if (trackInfoBackup[j].folderName == folderName) {
            backup = &trackInfoBackup[j];
            break;
        }
    }

    if (backup != nullptr) {
        // Copy relevant data
        track->challengeTime = backup->challengeTime;
        track->challengeReverseTime = backup->challengeReverseTime;
        track->trackLengthNormal = backup->trackLengthNormal;
        track->trackLengthReverse = backup->trackLengthReverse;
    }

}


void Hook_LoadVanillaCups() {
    Orig_LoadVanillaCups();
}

void Hook_LoadCustomCups() {
    Orig_LoadCustomCups();
}

void Hook_UpdateCarSelectability() {

    // Inside your hook function:
    CarInfo* rawPool = *reinterpret_cast<CarInfo**>(AbsFromRva(RVA_CAR_TABLE));
    int32_t carCount = *reinterpret_cast<int32_t*>(AbsFromRva(RVA_CAR_COUNT));

    // Guard against accessing the pool before it is allocated by the game
    if (rawPool != nullptr && carCount > 0) {
        for (int32_t i = 0; i < carCount; ++i) {
            CarInfo& currentCar = rawPool[i];
            
            // Reapply rating and obtain condition to make sure
            // UpdateCarSelectability runs based on the randomized data
            ApplyCarMods(i, &currentCar, nullptr);

            // Example: Read or modify the selectableByPlayer boolean
            // if (currentCar.selectableByPlayer) { ... }
        }
    }

    Orig_UpdateCarSelectability();

}


} // namespace Randomizer
