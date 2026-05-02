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
// InitHardcodedCarPaths adds the "cars/" prefix to each string if it's missing.
// ============================================================================
static const char* defaultCars[49] = {
    "rc",  "mite",  "phat",   "moss",   "mud",    "beatall", "volken",
    "tc6", "dino",  "candy",  "gencar", "tc4",    "mouse",   "flag",
    "tc2", "r5",    "tc5",    "sgt",    "tc3",    "adeon",   "fone",
    "tc1", "rotor", "cougar", "sugo",   "toyeca", "amw",     "panga",

    "trolley", "wincar",   "wincar2", "wincar3", "wincar4",  "ufo",       "q",
    "bigvolt", "bossvolt", "jg6rc",   "tc12",    "tc10",     "tc8",       "tc11",
    "tc9",     "jg1jg7",   "tc7",     "jg3loco", "jg4snw35", "jg5purpxl", "jg2fulonx"
};

static const char* defaultTracks[14] = {
    "nhood1",  "market2",    "muse2",      "garden1",
    "roof",    "toylite",    "wild_west1", "toy2",
    "nhood2",  "ship1",      "muse1",
    "market1", "wild_west2", "ship2"
};

// 1. Add a backing array to guarantee the memory lifetime of the strings
static std::string s_patchedStrings[49];

// Persistent storage for the patched pointer table.
// The pointers in g_VanillaCarPaths must remain valid after the hook returns,
// so these are static rather than stack-allocated.
static const char* s_patchedPtrs[49];

void InitHardcodedCarPath(int index) {
    std::string carPath = defaultCars[index];

    // Note: Using standard .find() == 0 is safer and more portable across compilers 
    // than the MSVC-specific _Starts_with implementation.
    if (carPath.find("cars/") != 0) { 
        carPath = "cars/" + carPath; // Ensure the path has the correct prefix
    }
    
    // 2. Assign the generated string to the static backing array
    s_patchedStrings[index] = carPath;
    
    // 3. Point the raw char array to the safely stored string
    s_patchedPtrs[index] = s_patchedStrings[index].c_str();
}

void InitStockCarPaths() {
    for (int i = 0; i < 28; i++) {
        InitHardcodedCarPath(i);
    }
}

void InitDCCarPaths() {
    for (int i = 35; i < 49; i++) {
        InitHardcodedCarPath(i);
    }
}

void InitSpecialCarPaths() {
    for (int i = 28; i < 35; i++) {
        InitHardcodedCarPath(i);
    }
}

} // anonymous namespace

namespace Randomizer {

// Global or class-member variable to hold the active configuration
std::optional<ConfigData> g_ActiveConfig = std::nullopt;

void Initialize() {
    Logger::TimestampLogf("[Randomizer] Initializing Randomizer module...");

    // 1. Attempt to load the configuration file
    // Make sure the path matches where you place the test JSON relative to rvgl.exe
    // Replaced by environment variable passed from the Rust module
    //const std::string configPath = "randomizer_config.json"; 
    
    g_ActiveConfig = LoadConfiguration();

    // 2. Verify successful load and output test data
    if (g_ActiveConfig.has_value()) {
        Logger::TimestampLogf("[Randomizer] Successfully loaded configuration!");
        Logger::TimestampLogf("[Randomizer] Seed: %s", g_ActiveConfig->metadata.seed.c_str());
        Logger::TimestampLogf("[Randomizer] Parsed %zu stock cars, %zu DC cars, and %zu tracks.", 
            g_ActiveConfig->stockCars.size(), 
            g_ActiveConfig->dcCars.size(), 
            g_ActiveConfig->tracks.size());
        
        // Example of accessing nested data
        if (!g_ActiveConfig->stockCars.empty()) {
            Logger::TimestampLogf("[Randomizer] First stock car folder: %s (Rating: %d)", 
                g_ActiveConfig->stockCars[0].folder.c_str(), 
                g_ActiveConfig->stockCars[0].rating);
        }
            
        // Init s_patchedPtrs based on config
        for (size_t i = 0; i < g_ActiveConfig->stockCars.size() && i <= 27; ++i) {
            std::string carPath = g_ActiveConfig->stockCars[i].folder;
            if (!carPath._Starts_with("cars/")) {
                carPath = "cars/" + carPath; // Ensure the path has the correct prefix
            }
            g_ActiveConfig->stockCars[i].folder = carPath; // Update the folder in the config struct
            s_patchedPtrs[i] = g_ActiveConfig->stockCars[i].folder.c_str();
        }
        for (size_t i = 0; i < g_ActiveConfig->dcCars.size() && i <= 13; ++i) {
            std::string carPath = g_ActiveConfig->dcCars[i].folder;
            if (!carPath._Starts_with("cars/")) {
                carPath = "cars/" + carPath; // Ensure the path has the correct prefix
            }
            g_ActiveConfig->dcCars[i].folder = carPath; // Update the folder in the config struct
            s_patchedPtrs[35 + i] = g_ActiveConfig->dcCars[i].folder.c_str();
        }
    } else {
        Logger::TimestampLogf("[Randomizer] Failed to load or parse configuration. Mod will remain inactive.");
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
FnGetProfileIndex        Orig_GetProfileIndex        = nullptr;
FnProfile_CreateOrLoad   Orig_Profile_CreateOrLoad   = nullptr;
FnProfile_LoadAndReset   Orig_Profile_LoadAndReset   = nullptr;
FnLoadSettingsFromIni    Orig_LoadSettingsFromIni    = nullptr;
FnTrack_ApplyCustomUnlock Orig_Track_ApplyCustomUnlock = nullptr;
FnCheckIfTierChampionshipWon Orig_CheckIfTierChampionshipWon = nullptr;
FnCheckIfTierTimeTrialsBeaten Orig_CheckIfTierTimeTrialsBeaten = nullptr;
FnCheckIfTierPracticeStarsFound Orig_CheckIfTierPracticeStarsFound = nullptr;
FnCheckIfTierSingleRacesWon Orig_CheckIfTierSingleRacesWon = nullptr;

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

// Flag to enable/disable difficulty manipulation in unlock checks.
// Enabled only when we do a track check, since the check functions are shared between cars and tracks.
static bool checkingTrackUnlocks = false;

// ============================================================================
// Hook_LoadCars
//
// Fires after RVGL has loaded all cars into the pool.
// We call through to the original first so the pool is fully built,
// then snapshot the pool pointer and count for use in Hook_BuildGrid.
// ============================================================================
bool Hook_LoadVanillaCarPool() {

    size_t initialCarCount = 49; // Vanilla RVGL has 49 cars in the pool, so we expect this many to be loaded before our mods apply.

    if (g_ActiveConfig.has_value() && !g_ActiveConfig->stockCars.empty()) {
        Logger::TimestampLogf("[Randomizer] Using initialized stock cars from config.");
    }
    else {
        Logger::TimestampLogf("[Randomizer] No stock cars specified in config, defaulting to hardcoded paths for first 28 cars.");
        InitStockCarPaths();
    }

    Logger::TimestampLogf("[Randomizer] Initializing special car paths for cars 28-34.");
    InitSpecialCarPaths();

    if (g_ActiveConfig.has_value() && !g_ActiveConfig->dcCars.empty()) {
        Logger::TimestampLogf("[Randomizer] Using initialized DC cars from config.");
    }
    else {
        Logger::TimestampLogf("[Randomizer] No DC cars specified in config, defaulting to hardcoded paths for last 14 cars.");
        InitDCCarPaths();
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
    std::string vfsPath = "";
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

                // Absolute path for writing the generated carbox, which we'll delete after loading
                customTexturePath = GetCarboxAbsoluteFolderPath() + "\\custom_carbox_" + internalName + ".bmp";
                GenerateAndSaveSingleCarbox(customTexturePath, src);

                // keep original VFS path for the engine
                targetPath = path;
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
                
                std::string filename = "carbox_random_" + std::to_string(carboxNumber) + ".bmp";

                // Write to the correct pack directory using an absolute path
                customTexturePath = GetCarboxAbsoluteFolderPath() + "\\" + filename;

                // Generate the stitched file to disk
                GenerateAndSaveCarboxAtlas(customTexturePath, randomGrid);

                Logger::TimestampLogf("[LoadTextureByName] Atlas carbox %d written to: %s", carboxNumber, customTexturePath.c_str());

                // Point our targetPath to the newly generated file instead of overwriting memory
                // Redirect the engine to the VFS-relative path so it finds the file
                vfsPath = "cars/misc/" + filename;
                targetPath = const_cast<char*>(vfsPath.c_str());
            }
        }
    }

    Logger::TimestampLogf("[LoadTextureByName] Calling on path %s for slot ID %d", targetPath, slotID);
                
    // Call the original texture loader
    unsigned long long returnValue = Orig_LoadTextureByName(targetPath, slotID, maxMipLevel, enableMips, param_5, flags);

    if (customTexturePath != "") {
        // delete random carbox bmp - absolute path given
        std::remove(customTexturePath.c_str());
    }
    return returnValue;
}


void Hook_LoadCustomCarPool() {

    // If the config explicitly says not to load extra cars,
    // skip calling the original function which loads them from disk.
    if (g_ActiveConfig.has_value() && !g_ActiveConfig->global_options.load_extra_cars) {
        return;
    }

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
    }

    // If the car is stock/dc and has statistics disabled, enable them
    if (IsStockCar(carName)) {
        car->statisticsEnabled = true;
        if (physData != nullptr) {
            physData->statistics = true;
        }
    }

    if (g_ActiveConfig.has_value()) {
        RandomizedCar* carConfigPtr = nullptr;
        if (carIndex >= 0 && carIndex <= 27) {
            if (carIndex < g_ActiveConfig->stockCars.size()) {
                carConfigPtr = &g_ActiveConfig->stockCars[carIndex];
            }
        } else if (carIndex >= 35 && carIndex <= 48) {
            int dcIndex = carIndex - 35;
            if (dcIndex < g_ActiveConfig->dcCars.size()) {
                carConfigPtr = &g_ActiveConfig->dcCars[dcIndex];
            }
        }

        if (carConfigPtr != nullptr) {
            RandomizedCar& carConfig = *carConfigPtr;
            
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
            
            std::string folderName = currentTrack->folderName;

            // 2. Apply randomization
            if (g_ActiveConfig.has_value() && i < g_ActiveConfig->tracks.size()) {

                folderName = g_ActiveConfig->tracks[i].folder;
            }
            else {
                folderName = defaultTracks[i];
            }
            strncpy_s(currentTrack->folderName, 16, folderName.c_str(), _TRUNCATE);
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

    for (int i = 0; i < 14; i++) {
        TrackInfo* currentTrack = &vanillaTracks[i];
        Logger::TimestampLogf("[LoadVanillaTracks] Track %d: %s", i+1, currentTrack->displayName);
        
        // Apply missing hardcoded data
        ApplyStockTrackData(currentTrack);

        // Apply difficulty rating from config
        if (g_ActiveConfig.has_value() && i < g_ActiveConfig->tracks.size()) {
            currentTrack->difficultyRating = g_ActiveConfig->tracks[i].difficulty;
            currentTrack->obtainCondition = g_ActiveConfig->tracks[i].obtain;
        }
    }
}

void Hook_LoadCustomTracks() {

    // If the config explicitly says not to load extra tracks,
    // skip calling the original function which loads them from disk.
    if (g_ActiveConfig.has_value() && !g_ActiveConfig->global_options.load_extra_tracks) {
        return;
    }

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

    CupProfile* vanillaCups = reinterpret_cast<CupProfile*>(AbsFromRva(RVA_VANILLA_CUP_ARRAY));
    CupProfile* dcCups = reinterpret_cast<CupProfile*>(AbsFromRva(RVA_DC_CUP_ARRAY));

    Orig_LoadVanillaCups();

    // Apply init data after loading the original files
    for (int i = 0; i < 4; i++) {
        if (g_ActiveConfig.has_value() && i < g_ActiveConfig->cups.size()) {
            RandomizedCup& cupConfig = g_ActiveConfig->cups[i];

            CupProfile* vanillaCup = &vanillaCups[i+1]; // +1 because index 0 is empty in the vanilla array
            CupProfile* dcCup = &dcCups[i]; // DC array is 0-indexed

            vanillaCup->obtainCondition      = dcCup->obtainCondition      = cupConfig.obtainCondition;
            vanillaCup->difficultyRating     = dcCup->difficultyRating     = cupConfig.difficulty;
            vanillaCup->numCars              = dcCup->numCars              = cupConfig.numCars;
            vanillaCup->numTries             = dcCup->numTries             = cupConfig.numTries;
            vanillaCup->perRaceRequiredPlace = dcCup->perRaceRequiredPlace = cupConfig.perRaceRequiredPlace;
            vanillaCup->overallRequiredPlace = dcCup->overallRequiredPlace = cupConfig.overallRequiredPlace;
            vanillaCup->numStages            = dcCup->numStages            = cupConfig.stages.size();
            
            if (cupConfig.carsPerClass.size() >= 6) {
                vanillaCup->maxRookie   = dcCup->maxRookie   = cupConfig.carsPerClass[0];
                vanillaCup->maxAmateur  = dcCup->maxAmateur  = cupConfig.carsPerClass[1];
                vanillaCup->maxAdvanced = dcCup->maxAdvanced = cupConfig.carsPerClass[2];
                vanillaCup->maxSemiPro  = dcCup->maxSemiPro  = cupConfig.carsPerClass[3];
                vanillaCup->maxPro      = dcCup->maxPro      = cupConfig.carsPerClass[4];
                vanillaCup->maxSuperPro = dcCup->maxSuperPro = cupConfig.carsPerClass[5];
            }

            if (cupConfig.pointsTable.size() >= 16) {
                for (int j = 0; j < 16; j++) {
                    vanillaCup->pointsTable[j] = dcCup->pointsTable[j] = cupConfig.pointsTable[j];
                }
            }

            for (size_t stageIndex = 0; stageIndex < cupConfig.stages.size() && stageIndex < 16; stageIndex++) {
                const RandomizedCupStage& stageConfig = cupConfig.stages[stageIndex];
                CupStage* vanillaStage = &vanillaCup->stages[stageIndex];
                CupStage* dcStage = &dcCup->stages[stageIndex];

                const char* trackName = stageConfig.trackFolder.c_str();

                int trackId = -1;
                for (int t = 0; t < s_trackCount; t++) {
                    if (strcmp(s_vanillaTrackPool[t].folderName, trackName) == 0) {
                        trackId = t;
                        break;
                    }
                }

                if (trackId != -1) {
                    vanillaStage->trackID = dcStage->trackID = trackId;
                } else {
                    Logger::TimestampLogf("[LoadVanillaCups] Warning: Track folder '%s' not found for cup '%s' stage %d", trackName, cupConfig.name.c_str(), stageIndex+1);
                }
                vanillaStage->numLaps = dcStage->numLaps = stageConfig.numLaps;
                vanillaStage->isReverse = dcStage->isReverse = stageConfig.isReverse;
                vanillaStage->isMirror = dcStage->isMirror = stageConfig.isMirror;
            }
        }
    }
}

void Hook_LoadCustomCups() {

    // If the config explicitly says not to load extra cups,
    // skip calling the original function which loads them from disk.
    if (g_ActiveConfig.has_value() && !g_ActiveConfig->global_options.load_extra_cups) {
        return;
    }

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

bool skipNextProfileLoad = false;

int Hook_GetProfileIndex(char* profileName) {
    int index = Orig_GetProfileIndex(profileName);
    if (index == -1) {
        Orig_Profile_CreateOrLoad(profileName);
        index = Orig_GetProfileIndex(profileName);
        
        // Profile_CreateOrLoad already calls Profile_LoadAndReset,
        // so we can skip it the next time it's called
        skipNextProfileLoad = true;
    }
    return index;
}

bool Hook_Profile_CreateOrLoad(char* displayName) {
    return Orig_Profile_CreateOrLoad(displayName);
}

void Hook_Profile_LoadAndReset(char* profileName) {
    // Profile already loaded in GetProfileIndex
    // This happens when we are loading a profile created
    // for the randomizer for the first time
    if (skipNextProfileLoad) {
        skipNextProfileLoad = false;
        return;
    }
    Orig_Profile_LoadAndReset(profileName);
}

void Hook_LoadSettingsFromIni(char* profileName) {
    Orig_LoadSettingsFromIni(profileName);

    // Force CupDC flag to true so DC cups are loaded and selectable in championships.
    bool* cupDCFlag = reinterpret_cast<bool*>(AbsFromRva(RVA_CUP_DC));
    *cupDCFlag = true;
}

void Hook_Track_ApplyCustomUnlock(int trackIndex) {
    checkingTrackUnlocks = true; // Set the flag to indicate we're in a track unlock check
    Orig_Track_ApplyCustomUnlock(trackIndex);
    checkingTrackUnlocks = false; // Reset the flag after the unlock check
}

bool Hook_CheckIfTierChampionshipWon(int difficultyRating) {
    int actualDifficulty = checkingTrackUnlocks ? difficultyRating - 1 : difficultyRating;
    bool result = Orig_CheckIfTierChampionshipWon(actualDifficulty);
    return result;
}

bool Hook_CheckIfTierTimeTrialsBeaten(int difficultyRating) {
    int actualDifficulty = checkingTrackUnlocks ? difficultyRating - 1 : difficultyRating;
    bool result = Orig_CheckIfTierTimeTrialsBeaten(actualDifficulty);
    return result;
}

bool Hook_CheckIfTierPracticeStarsFound(int difficultyRating) {
    int actualDifficulty = checkingTrackUnlocks ? difficultyRating - 1 : difficultyRating;
    bool result = Orig_CheckIfTierPracticeStarsFound(actualDifficulty);
    return result;
}

bool Hook_CheckIfTierSingleRacesWon(int difficultyRating) {
    int actualDifficulty = checkingTrackUnlocks ? difficultyRating - 1 : difficultyRating;
    bool result = Orig_CheckIfTierSingleRacesWon(actualDifficulty);
    return result;
}

} // namespace Randomizer
