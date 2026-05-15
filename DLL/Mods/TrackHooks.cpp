#include "TrackHooks.h"
#include "CupHooks.h"
#include "CustomUnlocks.h"
#include "RandomizerState.h"
#include "Addresses.h"
#include "RVGLFunctions.h"
#include "Logger.h"
#include <cstring>
#include <unordered_set>

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

int GetRuntimeTrackCount() {
    return *reinterpret_cast<int*>(AbsFromRva(RVA_TRACK_COUNT));
}

TrackInfo* GetVanillaTrackArray() {
    return reinterpret_cast<TrackInfo*>(AbsFromRva(RVA_VANILLA_TRACKS_TABLE));
}

TrackInfo* GetCustomTrackArray() {
    return *reinterpret_cast<TrackInfo**>(AbsFromRva(RVA_CUSTOM_TRACKS_TABLE));
}

TrackInfo* GetTrackInfoByRuntimeIndex(int trackIndex) {
    if (trackIndex < 0) {
        return nullptr;
    }

    const int trackCount = GetRuntimeTrackCount();
    if (trackIndex >= trackCount) {
        return nullptr;
    }

    if (trackIndex < 21) {
        TrackInfo* vanillaTracks = GetVanillaTrackArray();
        return vanillaTracks != nullptr ? &vanillaTracks[trackIndex] : nullptr;
    }

    TrackInfo* customTracks = GetCustomTrackArray();
    return customTracks != nullptr ? &customTracks[trackIndex - 21] : nullptr;
}

int FindTrackIdByFolderName(const std::string& folderName) {
    if (folderName.empty()) {
        return -1;
    }

    return FindTrackIdByFolderName(folderName.c_str());
}

int FindTrackIdByFolderName(const char* trackName) {
    if (trackName == nullptr || trackName[0] == '\0') {
        return -1;
    }

    const int trackCount = GetRuntimeTrackCount();
    for (int trackIndex = 0; trackIndex < trackCount; ++trackIndex) {
        TrackInfo* track = GetTrackInfoByRuntimeIndex(trackIndex);
        if (track != nullptr && _stricmp(track->folderName, trackName) == 0) {
            return trackIndex;
        }
    }

    return -1;
}

bool TrackFileExists(int trackIndex) {
    return RVGL_TrackFileExists(trackIndex) != 0;
}

RandomizedTrack* GetTrackConfigByRuntimeIndex(int trackIndex) {
    ConfigData* config = GetActiveConfig();
    if (config == nullptr || trackIndex < 0 || trackIndex >= 14) {
        return nullptr;
    }

    const bool useCupDC = GetRandomizerContext().config.useCupDC;
    if (!useCupDC && trackIndex == 4) {
        return nullptr;
    }

    int configIndex = trackIndex;
    if (!useCupDC && trackIndex > 4) {
        configIndex = trackIndex - 1;
    }

    if (configIndex < 0 || configIndex >= static_cast<int>(config->tracks.size())) {
        return nullptr;
    }

    return &config->tracks[configIndex];
}

const CustomUnlockCondition* GetTrackCustomUnlockCondition(int trackIndex) {
    RandomizedTrack* trackConfig = GetTrackConfigByRuntimeIndex(trackIndex);
    if (trackConfig == nullptr || !trackConfig->customUnlock.has_value()) {
        return nullptr;
    }

    return &trackConfig->customUnlock.value();
}

void ApplyUnlockedTrackAvailability(int trackIndex, TrackInfo* track) {
    if (track == nullptr) {
        return;
    }

    track->trackAvailFlags = static_cast<TrackAvailFlags>(track->trackAvailFlags | TRACKAVAIL_NORMAL);

    if (!RVGL_TrackReversedDirExists(trackIndex)) {
        track->trackAvailFlags = static_cast<TrackAvailFlags>(track->trackAvailFlags | TRACKAVAIL_REVERSE);
        return;
    }

    const bool normalChallengeSatisfied =
        track->challengeTime == 0 ||
        (track->trackProgressFlags & TRACKPROGRESS_NORMAL_CHALLENGE_BEATEN) != 0;

    const bool reverseChallengeSatisfied =
        track->challengeReverseTime == 0 ||
        (track->trackProgressFlags & TRACKPROGRESS_REVERSE_CHALLENGE_BEATEN) != 0;

    const bool mirrorChallengeSatisfied =
        track->challengeTime == 0 ||
        (track->trackProgressFlags & TRACKPROGRESS_MIRROR_CHALLENGE_BEATEN) != 0;

    if (normalChallengeSatisfied) {
        track->trackAvailFlags = static_cast<TrackAvailFlags>(track->trackAvailFlags | TRACKAVAIL_TT);
    }

    if (reverseChallengeSatisfied) {
        track->trackAvailFlags = static_cast<TrackAvailFlags>(track->trackAvailFlags | TRACKAVAIL_REVERSE);
    }

    if (normalChallengeSatisfied && reverseChallengeSatisfied && mirrorChallengeSatisfied) {
        track->trackAvailFlags = static_cast<TrackAvailFlags>(track->trackAvailFlags | TRACKAVAIL_MIRROR);
    }
}

void LogMissingCustomTrackUnlockOnce(int trackIndex, const TrackInfo& track) {
    static std::unordered_set<int> loggedTrackIndices;
    if (!loggedTrackIndices.insert(trackIndex).second) {
        return;
    }

    Logger::TimestampLogf(
        "[Track_ApplyCustomUnlock] Warning: Custom unlock obtain %d for track %d ('%s') has no customUnlock config.",
        static_cast<int>(track.obtainCondition),
        trackIndex,
        track.folderName
    );
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

        // Copy difficulty rating
        // TO DO: Distinguish between custom tracks with randomized attributes and
        // custom tracks which are being loaded as extra tracks
        track->difficultyRating = backup->difficultyRating;
    }

}

// Flag to enable/disable difficulty manipulation in unlock checks.
// Enabled only when we do a track check, since the check functions are shared between cars and tracks.
static bool checkingTrackUnlocks = false;

void Hook_Track_ApplyCustomUnlock(int trackIndex) {
    TrackInfo* track = GetTrackInfoByRuntimeIndex(trackIndex);
    if (track == nullptr) {
        return;
    }

    const int32_t obtain = static_cast<int32_t>(track->obtainCondition);
    if (IsDefaultObtain(obtain)) {
        checkingTrackUnlocks = true; // Set the flag to indicate we're in a track unlock check
        Orig_Track_ApplyCustomUnlock(trackIndex);
        checkingTrackUnlocks = false; // Reset the flag after the unlock check
        return;
    }

    if (!TrackFileExists(trackIndex)) {
        return;
    }

    const CustomUnlockCondition* customUnlock = GetTrackCustomUnlockCondition(trackIndex);
    if (customUnlock == nullptr) {
        LogMissingCustomTrackUnlockOnce(trackIndex, *track);
        return;
    }

    if (!EvaluateCustomUnlock(UnlockTargetKind::Track, trackIndex, obtain, customUnlock)) {
        return;
    }

    ApplyUnlockedTrackAvailability(trackIndex, track);
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
