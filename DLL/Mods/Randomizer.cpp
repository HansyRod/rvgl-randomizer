#include "Randomizer.h"
#include "RandomizerState.h"
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

static const char* defaultTracks[14] = {
    "nhood1",  "market2",    "muse2",      "garden1",
    "roof",    "toylite",    "wild_west1", "toy2",
    "nhood2",  "ship1",      "muse1",
    "market1", "wild_west2", "ship2"
};

} // anonymous namespace

namespace Randomizer {

// ----------------------------------------------------------------------------
// Original function pointers
// MinHook writes the trampoline addresses into these during InstallAll().
// ----------------------------------------------------------------------------
FnLoadVanillaCups        Orig_LoadVanillaCups        = nullptr;
FnLoadCustomCups         Orig_LoadCustomCups         = nullptr;
FnGetProfileIndex        Orig_GetProfileIndex        = nullptr;
FnProfile_CreateOrLoad   Orig_Profile_CreateOrLoad   = nullptr;
FnProfile_LoadAndReset   Orig_Profile_LoadAndReset   = nullptr;
FnLoadSettingsFromIni    Orig_LoadSettingsFromIni    = nullptr;
FnInitCarPhysicsBlock       Orig_InitCarPhysicsBlock       = nullptr;
FnToken_Matches             Orig_Token_Matches             = nullptr;
FnReadTokenFloat            Orig_ReadTokenFloat            = nullptr;
FnReadTokenInt              Orig_ReadTokenInt              = nullptr;
FnReadTokenBool             Orig_ReadTokenBool             = nullptr;

void Hook_LoadVanillaCups() {

    ConfigData* config = GetActiveConfig();
    RandomizerContext& ctx = GetRandomizerContext();
    TrackRuntimeState& trackState = ctx.trackState;

    CupProfile* vanillaCups = reinterpret_cast<CupProfile*>(AbsFromRva(RVA_VANILLA_CUP_ARRAY));
    CupProfile* dcCups = reinterpret_cast<CupProfile*>(AbsFromRva(RVA_DC_CUP_ARRAY));

    Orig_LoadVanillaCups();

    // Apply init data after loading the original files
    for (int i = 0; i < 4; i++) {
        if (config != nullptr && i < config->cups.size()) {
            RandomizedCup& cupConfig = config->cups[i];

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
                for (int t = 0; t < trackState.trackCount; t++) {
                    if (strcmp(trackState.vanillaTrackPool[t].folderName, trackName) == 0) {
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
    ConfigData* config = GetActiveConfig();
    if (config != nullptr && !config->global_options.load_extra_cups) {
        return;
    }

    Orig_LoadCustomCups();
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

    RandomizerContext& ctx = GetRandomizerContext();
    bool useCupDC = ctx.config.useCupDC;

    Orig_LoadSettingsFromIni(profileName);

    // Set CupDC flag according to the config info.
    bool* cupDCFlag = reinterpret_cast<bool*>(AbsFromRva(RVA_CUP_DC));
    *cupDCFlag = useCupDC;
}

int spinnerType = 0;
float spinnerAngVel = 0.0f;
bool flippable = false;

bool checkSpinner = false;
bool checkFlippable = false;

void Hook_InitCarPhysicsBlock(CarPhysicsData *physData, int carIndex) {
    
    RandomizerContext& ctx = GetRandomizerContext();

    // Reset temp values to their defaults;
    spinnerType = 0;
    spinnerAngVel = 0.0f;
    flippable = false;

    // Get current car folder
    CarInfo* carPool = ctx.carState.carPool.data();
    CarInfo* car = &carPool[carIndex];
    char* carName = car->internalName;

    // Store the original values before the game overwrites them
    if (strcmp(carName, "panga") != 0 && carIndex == 27) {
        checkSpinner = true;
    }
    else {
        checkSpinner = false;
    }

    if (strcmp(carName, "rotor") != 0 && carIndex == 22) {
        checkFlippable = true;
    }
    else {
        checkFlippable = false;
    }
    
    Orig_InitCarPhysicsBlock(physData, carIndex);

    // Set spinner props for panga, or for the car in panga's slot
    if (strcmp(carName, "panga") == 0 && carIndex != 27) {
        physData->spinnerType = 6;
        physData->spinnerAngVel = 1.5;
    }
    else if (checkSpinner) {
        physData->spinnerType = spinnerType;
        physData->spinnerAngVel = spinnerAngVel;
    }

    // Set flippable prop for rotor, or for the car in rotor's slot
    if (strcmp(carName, "rotor") == 0 && carIndex != 22) {
        physData->flippable = true;
    }
    else if (checkFlippable) {
        physData->flippable = flippable;
    }
}

bool storeNextFloat = false;
bool storeNextInt = false;
bool storeNextBool = false;

bool Hook_Token_Matches(char* token, char* str) {

    bool result = Orig_Token_Matches(token, str);

    if (result) {
        if (checkSpinner) {
            if (strcmp(str, "TYPE") == 0) {
                storeNextInt = true;
            }
            else if (strcmp(str, "ANGVEL") == 0) {
                storeNextFloat = true;
            }
        }
        if (checkFlippable) {
            if (strcmp(str, "FLIPPABLE") == 0) {
                storeNextBool = true;
            }
        }
    }

    return result;
}

bool Hook_ReadTokenFloat(float* outValue, FILE* file) {
    bool result = Orig_ReadTokenFloat(outValue, file);
    if (result && storeNextFloat) {
        spinnerAngVel = *outValue;
        storeNextFloat = false;
    }
    return result;
}

bool Hook_ReadTokenInt(int* outValue, FILE* file) {
    bool result = Orig_ReadTokenInt(outValue, file);
    if (result && storeNextInt) {
        spinnerType = *outValue;
        storeNextInt = false;
    }
    return result;
}

bool Hook_ReadTokenBool(bool* outValue, FILE* file) {
    bool result = Orig_ReadTokenBool(outValue, file);
    if (result && storeNextBool) {
        flippable = *outValue;
        storeNextBool = false;
    }
    return result;
}

} // namespace Randomizer
