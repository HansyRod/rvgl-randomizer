#include "CupHooks.h"
#include "RandomizerState.h"
#include "Addresses.h"
#include "Logger.h"

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

} // namespace Randomizer
