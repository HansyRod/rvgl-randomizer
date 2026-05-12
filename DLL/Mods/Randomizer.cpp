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
FnLoadVanillaTracks      Orig_LoadVanillaTracks      = nullptr;
FnLoadCustomTracks       Orig_LoadCustomTracks       = nullptr;
FnLoadVanillaCups        Orig_LoadVanillaCups        = nullptr;
FnLoadCustomCups         Orig_LoadCustomCups         = nullptr;
FnGetProfileIndex        Orig_GetProfileIndex        = nullptr;
FnProfile_CreateOrLoad   Orig_Profile_CreateOrLoad   = nullptr;
FnProfile_LoadAndReset   Orig_Profile_LoadAndReset   = nullptr;
FnLoadSettingsFromIni    Orig_LoadSettingsFromIni    = nullptr;
FnTrack_ApplyCustomUnlock Orig_Track_ApplyCustomUnlock = nullptr;
FnCheckIfTierChampionshipWon Orig_CheckIfTierChampionshipWon = nullptr;
FnCheckIfTierTimeTrialsBeaten Orig_CheckIfTierTimeTrialsBeaten = nullptr;
FnCheckIfTierPracticeStarsFound Orig_CheckIfTierPracticeStarsFound = nullptr;
FnCheckIfTierSingleRacesWon Orig_CheckIfTierSingleRacesWon = nullptr;
FnInitCarPhysicsBlock       Orig_InitCarPhysicsBlock       = nullptr;
FnToken_Matches             Orig_Token_Matches             = nullptr;
FnReadTokenFloat            Orig_ReadTokenFloat            = nullptr;
FnReadTokenInt              Orig_ReadTokenInt              = nullptr;
FnReadTokenBool             Orig_ReadTokenBool             = nullptr;

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

void Hook_LoadVanillaTracks() {

    ConfigData* config = GetActiveConfig();
    RandomizerContext& ctx = GetRandomizerContext();
    bool useCupDC = ctx.config.useCupDC;

    TrackInfo* vanillaTracks = reinterpret_cast<TrackInfo*>(AbsFromRva(RVA_VANILLA_TRACKS_TABLE));
    
    // You can now access it like a standard array
    for (int i = 0; i < 21; i++) {
        TrackInfo* currentTrack = &vanillaTracks[i];

        if (i < 14) {
            // 1. Create a deep copy of the hardcoded data
            trackInfoBackup[i] = *currentTrack;

            // Skip if cupDC is false and we're in index 4 (Rooftops)
            if (i == 4 && !useCupDC) {
                Logger::TimestampLogf("[LoadVanillaTracks] Skipping folder patch for track %d (cupDC disabled)", i+1);
                continue;
            }
            
            std::string folderName = currentTrack->folderName;

            int actualIdx = i;
            // If cupDC is disabled and we're at index 4 or above, shift the index
            if (!useCupDC && i >= 4) {
                actualIdx = i - 1; // Shift back by one to skip the rooftops track config
            }

            // 2. Apply randomization
            if (config != nullptr && actualIdx < config->tracks.size()) {
                folderName = config->tracks[actualIdx].folder;
            }
            else {
                // Default tracks array includes rooftops, so the index doesn't need to be shifted here
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

        int actualIdx = i;
        // If cupDC is disabled and we're at index 4 or above, shift the index
        if (!useCupDC && i >= 4) {
            actualIdx = i - 1; // Shift back by one to skip the rooftops track config
        }

        // Apply difficulty rating from config
        if (config != nullptr && actualIdx < config->tracks.size()) {
            currentTrack->difficultyRating = config->tracks[actualIdx].difficulty;
            currentTrack->obtainCondition = config->tracks[actualIdx].obtain;
        }
    }
}

void Hook_LoadCustomTracks() {

    ConfigData* config = GetActiveConfig();

    // If the config explicitly says not to load extra tracks,
    // skip calling the original function which loads them from disk.
    if (config != nullptr && !config->global_options.load_extra_tracks) {
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

    ConfigData* config = GetActiveConfig();

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
