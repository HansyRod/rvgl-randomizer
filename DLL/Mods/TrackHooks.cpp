#include "TrackHooks.h"
#include "CupHooks.h"
#include "RandomizerState.h"
#include "Addresses.h"
#include "Logger.h"
#include <cstring>

// ============================================================================
// TrackHooks.cpp
//
// All hooks related to track loading and track unlock conditions.
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
FnTrack_ApplyCustomUnlock Orig_Track_ApplyCustomUnlock = nullptr;
FnCheckIfTierChampionshipWon Orig_CheckIfTierChampionshipWon = nullptr;
FnCheckIfTierTimeTrialsBeaten Orig_CheckIfTierTimeTrialsBeaten = nullptr;
FnCheckIfTierPracticeStarsFound Orig_CheckIfTierPracticeStarsFound = nullptr;
FnCheckIfTierSingleRacesWon Orig_CheckIfTierSingleRacesWon = nullptr;

int FindTrackIdByFolderName(const char* trackName) {
    if (trackName == nullptr || trackName[0] == '\0') {
        return -1;
    }

    const TrackRuntimeState& trackState = GetRandomizerContext().trackState;

    for (size_t trackId = 0; trackId < trackState.vanillaTrackPool.size(); trackId++) {
        const TrackInfo& currentTrack = trackState.vanillaTrackPool[trackId];
        if (_stricmp(currentTrack.folderName, trackName) == 0) {
            return static_cast<int>(trackId);
        }
    }

    for (size_t customIndex = 0; customIndex < trackState.customTrackPool.size(); customIndex++) {
        const TrackInfo& currentTrack = trackState.customTrackPool[customIndex];
        if (_stricmp(currentTrack.folderName, trackName) == 0) {
            return static_cast<int>(customIndex + 21);
        }
    }

    return -1;
}

void Hook_LoadVanillaTracks() {

    ConfigData* config = GetActiveConfig();
    RandomizerContext& ctx = GetRandomizerContext();
    TrackRuntimeState& trackState = ctx.trackState;
    bool useCupDC = ctx.config.useCupDC;

    TrackInfo* vanillaTracks = reinterpret_cast<TrackInfo*>(AbsFromRva(RVA_VANILLA_TRACKS_TABLE));
    
    // You can now access it like a standard array
    for (int i = 0; i < 21; i++) {
        TrackInfo* currentTrack = &vanillaTracks[i];

        if (i < 14) {
            // 1. Create a deep copy of the hardcoded data
            trackState.trackInfoBackup[i] = *currentTrack;

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
    trackState.trackCount = *reinterpret_cast<int*>(AbsFromRva(RVA_TRACK_COUNT));

    // Copy references to vanilla tracks
    trackState.vanillaTrackPool.clear();
    if (vanillaTracks != nullptr && trackState.trackCount > 0) {
        trackState.vanillaTrackPool.assign(vanillaTracks, vanillaTracks + trackState.trackCount);
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
    RandomizerContext& ctx = GetRandomizerContext();
    TrackRuntimeState& trackState = ctx.trackState;

    // If the config explicitly says not to load extra tracks,
    // skip calling the original function which loads them from disk.
    if (config != nullptr && !config->global_options.load_extra_tracks) {
        return;
    }

    Orig_LoadCustomTracks();

    TrackInfo* customTracksPool = *reinterpret_cast<TrackInfo**>(AbsFromRva(RVA_CUSTOM_TRACKS_TABLE));
    int trackCount = *reinterpret_cast<int*>(AbsFromRva(RVA_TRACK_COUNT));
    trackState.trackCount = trackCount;
    trackState.customTrackPool.clear();

    for (int i = 21; i < trackCount; i++) {
        TrackInfo* currentTrack = &customTracksPool[i-21];
        Logger::TimestampLogf("[LoadCustomTracks] Track %d: %s", i+1, currentTrack->displayName);
        ApplyStockTrackData(currentTrack);
    }

    // Copy the custom tracks pool only after stock track data is updated
    if (customTracksPool != nullptr && trackCount > 21) {
        trackState.customTrackPool.assign(customTracksPool, customTracksPool + (trackCount - 21));
    }

    // Default cups are first parsed before custom tracks exist. Re-parse them now
    // so cup stages can resolve custom track folder names to valid track IDs.
    if (Orig_LoadVanillaCups != nullptr) {
        Logger::TimestampLog("[LoadCustomTracks] Reloading default cups after custom tracks");
        Hook_LoadVanillaCups();
    }
}

void ApplyStockTrackData(TrackInfo* track) {

    RandomizerContext& ctx = GetRandomizerContext();
    TrackInfo* trackInfoBackup = ctx.trackState.trackInfoBackup;

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

// Flag to enable/disable difficulty manipulation in unlock checks.
// Enabled only when we do a track check, since the check functions are shared between cars and tracks.
static bool checkingTrackUnlocks = false;

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
