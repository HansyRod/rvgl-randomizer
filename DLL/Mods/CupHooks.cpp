#include "CupHooks.h"
#include "TrackHooks.h"
#include "CustomUnlocks.h"
#include "RandomizerState.h"
#include "Addresses.h"
#include "Logger.h"
#include <algorithm>
#include <unordered_set>

// ============================================================================
// CupHooks.cpp
//
// Hooks for modifying championship cup data, including unlock conditions and stage composition.
// ============================================================================

namespace Randomizer {

// ----------------------------------------------------------------------------
// Original function pointers
// MinHook writes the trampoline addresses into these during InstallAll().
// ----------------------------------------------------------------------------
FnLoadVanillaCups        Orig_LoadVanillaCups        = nullptr;
FnLoadCustomCups         Orig_LoadCustomCups         = nullptr;
FnCup_ValidateAndCheckUnlock Orig_Cup_ValidateAndCheckUnlock = nullptr;

int GetCustomCupCount() {
    return *reinterpret_cast<int*>(AbsFromRva(RVA_CUSTOM_CUPS_COUNT));
}

CupProfile* GetCustomCupArray() {
    return *reinterpret_cast<CupProfile**>(AbsFromRva(RVA_CUSTOM_CUP_ARRAY));
}

CupProfile* GetCupProfileByCupID(int cupID) {
    if (cupID < 1) {
        return nullptr;
    }

    if (cupID < 5) {
        const bool useCupDC = *reinterpret_cast<bool*>(AbsFromRva(RVA_CUP_DC));
        if (useCupDC) {
            CupProfile* dcCups = reinterpret_cast<CupProfile*>(AbsFromRva(RVA_DC_CUP_ARRAY));
            return &dcCups[cupID - 1];
        }

        CupProfile* vanillaCups = reinterpret_cast<CupProfile*>(AbsFromRva(RVA_VANILLA_CUP_ARRAY));
        return &vanillaCups[cupID];
    }

    const int customIndex = cupID - 5;
    if (customIndex < 0 || customIndex >= GetCustomCupCount()) {
        return nullptr;
    }

    CupProfile* customCups = GetCustomCupArray();
    return customCups != nullptr ? &customCups[customIndex] : nullptr;
}

RandomizedCup* GetCupConfigByCupID(int cupID) {
    ConfigData* config = GetActiveConfig();
    if (config == nullptr || cupID < 1) {
        return nullptr;
    }

    const int configIndex = cupID - 1;
    if (configIndex < 0 || configIndex >= static_cast<int>(config->cups.size())) {
        return nullptr;
    }

    return &config->cups[configIndex];
}

const CustomUnlockCondition* GetCupCustomUnlockCondition(int cupID) {
    RandomizedCup* cupConfig = GetCupConfigByCupID(cupID);
    if (cupConfig == nullptr || !cupConfig->customUnlock.has_value()) {
        return nullptr;
    }

    return &cupConfig->customUnlock.value();
}

void LogMissingCustomCupUnlockOnce(int cupID, const CupProfile& cup) {
    static std::unordered_set<int> loggedCupIDs;
    if (!loggedCupIDs.insert(cupID).second) {
        return;
    }

    Logger::TimestampLogf(
        "[Cup_ValidateAndCheckUnlock] Warning: Custom unlock obtain %d for cup %d ('%s') has no customUnlock config.",
        static_cast<int>(cup.obtainCondition),
        cupID,
        cup.internalName
    );
}

void RestoreCupExtendedValidationFieldsFromConfig(int cupID, CupProfile& cup) {
    if (!IsThirtyCarModeEnabled()) {
        return;
    }

    const RandomizedCup* cupConfig = GetCupConfigByCupID(cupID);
    if (cupConfig == nullptr || cupConfig->numCars <= 16) {
        return;
    }

    cup.numCars = cupConfig->numCars;
    cup.perRaceRequiredPlace = cupConfig->perRaceRequiredPlace;
    cup.overallRequiredPlace = cupConfig->overallRequiredPlace;

    if (cupConfig->carsPerClass.size() >= 6) {
        cup.maxRookie = cupConfig->carsPerClass[0];
        cup.maxAmateur = cupConfig->carsPerClass[1];
        cup.maxAdvanced = cupConfig->carsPerClass[2];
        cup.maxSemiPro = cupConfig->carsPerClass[3];
        cup.maxPro = cupConfig->carsPerClass[4];
        cup.maxSuperPro = cupConfig->carsPerClass[5];
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
            const int maxCarCount = IsThirtyCarModeEnabled()
                ? randomizerMaxCarCount
                : vanillaMaxCarCount;
            const int cupCarCount = (std::min)(cupConfig.numCars, maxCarCount);

            vanillaCup->obtainCondition      = dcCup->obtainCondition      = cupConfig.obtainCondition;
            vanillaCup->difficultyRating     = dcCup->difficultyRating     = cupConfig.difficulty;
            vanillaCup->numCars              = dcCup->numCars              = cupCarCount;
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
                int trackId = FindTrackIdByFolderName(trackName);

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

void Hook_Cup_ValidateAndCheckUnlock(int cupID) {
    CupProfile* cup = GetCupProfileByCupID(cupID);
    if (cup == nullptr) {
        Orig_Cup_ValidateAndCheckUnlock(cupID);
        return;
    }

    const int32_t originalObtain = static_cast<int32_t>(cup->obtainCondition);
    if (IsDefaultObtain(originalObtain)) {
        Orig_Cup_ValidateAndCheckUnlock(cupID);
        RestoreCupExtendedValidationFieldsFromConfig(cupID, *cup);
        return;
    }

    int* unlockChecksEnabled = reinterpret_cast<int*>(AbsFromRva(RVA_UNLOCK_CHECKS_ENABLED));
    const int savedUnlockChecksEnabled = *unlockChecksEnabled;

    cup->obtainCondition = UNLOCKED;
    *unlockChecksEnabled = 0;
    Orig_Cup_ValidateAndCheckUnlock(cupID);
    RestoreCupExtendedValidationFieldsFromConfig(cupID, *cup);
    *unlockChecksEnabled = savedUnlockChecksEnabled;
    cup->obtainCondition = static_cast<Obtain>(originalObtain);

    if (!cup->isUnlocked) {
        return;
    }

    const CustomUnlockCondition* customUnlock = GetCupCustomUnlockCondition(cupID);
    if (customUnlock == nullptr) {
        cup->isUnlocked = false;
        LogMissingCustomCupUnlockOnce(cupID, *cup);
        return;
    }

    cup->isUnlocked = EvaluateCustomUnlock(
        UnlockTargetKind::Cup,
        cupID,
        originalObtain,
        customUnlock
    );
}

} // namespace Randomizer
