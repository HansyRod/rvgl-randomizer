#include "CarHooks.h"
#include "RandomizerState.h"
#include "Logger.h"
#include "Addresses.h"
#include "Carbox.h"
#include "Image.h"
#include "GameUtils.h"
#include "CustomUnlocks.h"
#include <unordered_set>

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

void InitHardcodedCarPath(int index) {

    Randomizer::RandomizerContext& ctx = Randomizer::GetRandomizerContext();
    Randomizer::CarRuntimeState& carState = ctx.carState;
    const char** patchedPtrs = carState.patchedPtrs;
    std::string* patchedStrings = carState.patchedStrings;

    std::string carPath = defaultCars[index];

    // Note: Using standard .find() == 0 is safer and more portable across compilers 
    // than the MSVC-specific _Starts_with implementation.
    if (carPath.find("cars/") != 0) { 
        carPath = "cars/" + carPath; // Ensure the path has the correct prefix
    }
    
    // 2. Assign the generated string to the static backing array
    patchedStrings[index] = carPath;
    
    // 3. Point the raw char array to the safely stored string
    patchedPtrs[index] = patchedStrings[index].c_str();
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

}

namespace Randomizer {

// ----------------------------------------------------------------------------
// Original function pointers
// MinHook writes the trampoline addresses into these during InstallAll().
// ----------------------------------------------------------------------------
FnLoadVanillaCarPool     Orig_LoadVanillaCarPool     = nullptr;
FnLoadTextureByName      Orig_LoadTextureByName      = nullptr;
FnLoadCustomCarPool      Orig_LoadCustomCarPool      = nullptr;
FnSyncCarInfoFromPhysics Orig_SyncCarInfoFromPhysics = nullptr;
FnUpdateCarSelectability Orig_UpdateCarSelectability = nullptr;

RandomizedCar* GetCarConfigByRuntimeIndex(int carIndex) {
    ConfigData* config = GetActiveConfig();
    if (config == nullptr) {
        return nullptr;
    }

    if (carIndex >= 0 && carIndex <= 27) {
        if (carIndex < static_cast<int>(config->stockCars.size())) {
            return &config->stockCars[carIndex];
        }
    }
    else if (carIndex >= 35 && carIndex <= 48) {
        const int dcIndex = carIndex - 35;
        if (dcIndex < static_cast<int>(config->dcCars.size())) {
            return &config->dcCars[dcIndex];
        }
    }

    return nullptr;
}

int GetRuntimeCarCount() {
    return *reinterpret_cast<int*>(AbsFromRva(RVA_CAR_COUNT));
}

CarInfo* GetCarPool() {
    return *reinterpret_cast<CarInfo**>(AbsFromRva(RVA_CAR_TABLE));
}


// ============================================================================
// Hook_LoadCars
//
// Fires after RVGL has loaded all cars into the pool.
// We call through to the original first so the pool is fully built,
// then snapshot the pool pointer and count for use in Hook_BuildGrid.
// ============================================================================
bool Hook_LoadVanillaCarPool() {

    ConfigData* config = GetActiveConfig();
    RandomizerContext& ctx = GetRandomizerContext();
    CarRuntimeState& carState = ctx.carState;

    size_t initialCarCount = 49; // Vanilla RVGL has 49 cars in the pool, so we expect this many to be loaded before our mods apply.

    if (config != nullptr && !config->stockCars.empty()) {
        Logger::TimestampLogf("[Randomizer] Using initialized stock cars from config.");
    }
    else {
        Logger::TimestampLogf("[Randomizer] No stock cars specified in config, defaulting to hardcoded paths for first 28 cars.");
        InitStockCarPaths();
    }

    Logger::TimestampLogf("[Randomizer] Initializing special car paths for cars 28-34.");
    InitSpecialCarPaths();

    if (config != nullptr && !config->dcCars.empty()) {
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
    memcpy((void*)tableAddr, carState.patchedPtrs, tableSize);
    // restores the original protection (read-only again)
    VirtualProtect((void*)tableAddr, tableSize, old, &old);

    // Call the real LoadVanillaCarPool so RVGL loads its cars normally.
    const bool result = Orig_LoadVanillaCarPool();

    // Read the pool pointer and count that RVGL just populated.
    // AbsFromRva resolves the RVA to the actual runtime address, then we
    // dereference it to get the value stored at that address.
    CarInfo* rawPool = GetCarPool();
    carState.carCount = GetRuntimeCarCount();

    // Copy cars to s_carPool vector to keep them in the DLL memory
    carState.carPool.clear();
    if (rawPool != nullptr && carState.carCount > 0) {
        carState.carPool.assign(rawPool, rawPool + carState.carCount);
    }

    CarInfo* carPool = carState.carPool.data();
    for (int i = 0; i < carState.carCount; ++i) {
        CarInfo* car = &carPool[i];
        ApplyCarMods(i, car, nullptr);
    }

    Logger::TimestampLogf(("[Randomizer] Car pool snapshot — "
                        + std::to_string(carState.carCount) + " cars\n").c_str());

    return result;
}

unsigned long long Hook_LoadTextureByName(char* path, int slotID, int maxMipLevel, bool enableMips, int param_5, unsigned int flags) {

    RandomizerContext& ctx = GetRandomizerContext();
    std::vector<CarInfo>& carPool = ctx.carState.carPool;

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
            
            std::vector<CarboxSource> randomGrid = GetGridSourcesForCarbox(carboxNumber, carPool.data());

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

    ConfigData* config = GetActiveConfig();
    RandomizerContext& ctx = GetRandomizerContext();
    CarRuntimeState& carState = ctx.carState;

    // If the config explicitly says not to load extra cars,
    // skip calling the original function which loads them from disk.
    if (config != nullptr && !config->global_options.load_extra_cars) {
        return;
    }

    // Let RVGL load the custom cars from disk into memory first
    Orig_LoadCustomCarPool();

    CarInfo* customPool = GetCarPool();
    int customCount     = GetRuntimeCarCount();

    // Check if any cars were added to the car table
    if (customPool != nullptr && customCount > carState.carCount) {
        // Iterate only over the custom cars
        for (int i = carState.carCount; i < customCount; ++i) {
            CarInfo* car = &customPool[i];
            
            ApplyCarMods(i, car, nullptr);

            // Append to our persistent snapshot so GetGridSourcesForCarbox can find it
            carState.carPool.push_back(*car); 
        }
    }

    return;
}


void Hook_SyncCarInfoFromPhysics(int carIndex, CarPhysicsData *physData) {

    // Always apply car mods before syncing data. Not only the custom carboxes could be wrong, but also
    // obtain condition in DC cars that were moved to stock conditions was found with the original value
    // Applying car mods before sync guarantees that all unlock checks are done correctly
    RandomizerContext& ctx = GetRandomizerContext();
    std::vector<CarInfo>& carPool = ctx.carState.carPool;
    CarInfo* car = &carPool[carIndex];
    ApplyCarMods(carIndex, car, physData);

    Orig_SyncCarInfoFromPhysics(carIndex, physData);

}

void ApplyCarMods(int carIndex, CarInfo* car, CarPhysicsData *physData) {

    ConfigData* config = GetActiveConfig();

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

    if (config != nullptr) {
        RandomizedCar* carConfigPtr = GetCarConfigByRuntimeIndex(carIndex);

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

void Hook_UpdateCarSelectability() {

    CarInfo* rawPool = GetCarPool();
    int carCount = GetRuntimeCarCount();

    // Guard against accessing the pool before it is allocated by the game
    if (rawPool != nullptr && carCount > 0) {
        for (int i = 0; i < carCount; ++i) {
            CarInfo& currentCar = rawPool[i];
            
            // Reapply rating and obtain condition to make sure
            // UpdateCarSelectability runs based on the randomized data
            ApplyCarMods(i, &currentCar, nullptr);
        }
    }

    Orig_UpdateCarSelectability();

    UpdateCarCustomUnlocks();
}

}
