#include "ProgressFlagHooks.h"
#include "RVGLAddresses.h"
#include "Logger.h"
#include "TrackHooks.h"
#include <array>
#include <cstdint>

namespace {

constexpr int kStockProgressTrackCount = 14;
constexpr int kGameModeCurrentTrackOffset = 0x08;
constexpr int kStuntProgressCaughtCountOffset = 0x10;
constexpr int kStuntProgressMaxCountOffset = 0x14;
constexpr int kStuntProgressCaughtIdsOffset = 0x18;

enum class ProgressEventKind {
    TimeTrialChallengeBeaten,
    PracticeStarFound,
    SingleRaceWon,
    ChampionshipWon,
    StuntArenaStarCaught
};

const char* GetProgressEventName(ProgressEventKind kind) {
    switch (kind) {
    case ProgressEventKind::TimeTrialChallengeBeaten:
        return "TimeTrialChallengeBeaten";
    case ProgressEventKind::PracticeStarFound:
        return "PracticeStarFound";
    case ProgressEventKind::SingleRaceWon:
        return "SingleRaceWon";
    case ProgressEventKind::ChampionshipWon:
        return "ChampionshipWon";
    case ProgressEventKind::StuntArenaStarCaught:
        return "StuntArenaStarCaught";
    default:
        return "Unknown";
    }
}

int GetCurrentGameModeValue() {
    return *reinterpret_cast<int*>(AbsFromRva(RVA_GAME_MODE));
}

int GetCurrentTrackIndex() {
    return *reinterpret_cast<int*>(AbsFromRva(RVA_GAME_MODE) + kGameModeCurrentTrackOffset);
}

uint8_t* GetTrackProgressCache() {
    return RVGL::GetTrackProgressCache();
}

uint32_t GetTrackProgressFlags(int trackIndex) {
    TrackInfo* track = Randomizer::GetTrackInfoByRuntimeIndex(trackIndex);
    if (track == nullptr) {
        return 0;
    }

    return static_cast<uint32_t>(track->trackProgressFlags);
}

void EmitTrackProgressEvent(ProgressEventKind kind, int trackIndex, uint32_t flag) {
    TrackInfo* track = Randomizer::GetTrackInfoByRuntimeIndex(trackIndex);
    const char* folderName = track != nullptr ? track->folderName : "<unknown>";

    Logger::TimestampLogf(
        "[ProgressFlagHooks] %s track=%d folder='%s' flag=0x%08x",
        GetProgressEventName(kind),
        trackIndex,
        folderName,
        flag
    );
}

void EmitChampionshipWonEvent(int difficultyTier) {
    Logger::TimestampLogf(
        "[ProgressFlagHooks] %s difficultyTier=%d",
        GetProgressEventName(ProgressEventKind::ChampionshipWon),
        difficultyTier
    );
}

void EmitStuntArenaStarCaughtEvent(int starId) {
    Logger::TimestampLogf(
        "[ProgressFlagHooks] %s starId=%d",
        GetProgressEventName(ProgressEventKind::StuntArenaStarCaught),
        starId
    );
}

void EmitNewTrackFlagEvents(
    int trackIndex,
    uint32_t beforeFlags,
    uint32_t afterFlags,
    uint32_t watchedFlags,
    ProgressEventKind kind
) {
    const uint32_t newFlags = (~beforeFlags) & afterFlags & watchedFlags;
    if (newFlags == 0) {
        return;
    }

    for (uint32_t flag = 1; flag != 0; flag <<= 1) {
        if ((newFlags & flag) != 0) {
            EmitTrackProgressEvent(kind, trackIndex, flag);
        }
    }
}

} // anonymous namespace

namespace Randomizer {

FnUpdateTimeTrialLeaderboards Orig_UpdateTimeTrialLeaderboards = nullptr;
FnPickup_CollectProgressObject Orig_Pickup_CollectProgressObject = nullptr;
FnEngine_UpdateRaceProgress Orig_Engine_UpdateRaceProgress = nullptr;
FnCup_OnStageFinished Orig_Cup_OnStageFinished = nullptr;

void Hook_UpdateTimeTrialLeaderboards(int* carRaceData) {
    const int trackIndex = GetCurrentTrackIndex();
    const uint32_t beforeFlags = GetTrackProgressFlags(trackIndex);

    Orig_UpdateTimeTrialLeaderboards(carRaceData);

    const uint32_t afterFlags = GetTrackProgressFlags(trackIndex);
    constexpr uint32_t watchedFlags =
        TRACKPROGRESS_NORMAL_CHALLENGE_BEATEN |
        TRACKPROGRESS_REVERSE_CHALLENGE_BEATEN |
        TRACKPROGRESS_MIRROR_CHALLENGE_BEATEN;

    EmitNewTrackFlagEvents(
        trackIndex,
        beforeFlags,
        afterFlags,
        watchedFlags,
        ProgressEventKind::TimeTrialChallengeBeaten
    );
}

void Hook_Pickup_CollectProgressObject(void* pickup) {
    const int gameMode = GetCurrentGameModeValue();
    const int trackIndex = GetCurrentTrackIndex();
    const uint32_t beforeFlags = GetTrackProgressFlags(trackIndex);

    uint8_t* progressCache = GetTrackProgressCache();
    const int beforeStuntCount =
        gameMode == MODE_STUNT_ARENA && progressCache != nullptr
            ? *reinterpret_cast<int*>(progressCache + kStuntProgressCaughtCountOffset)
            : -1;

    Orig_Pickup_CollectProgressObject(pickup);

    if (gameMode == MODE_PRACTICE) {
        const uint32_t afterFlags = GetTrackProgressFlags(trackIndex);
        EmitNewTrackFlagEvents(
            trackIndex,
            beforeFlags,
            afterFlags,
            TRACKPROGRESS_PRACTICE_STAR,
            ProgressEventKind::PracticeStarFound
        );
        return;
    }

    if (gameMode != MODE_STUNT_ARENA || progressCache == nullptr || beforeStuntCount < 0) {
        return;
    }

    const int afterStuntCount =
        *reinterpret_cast<int*>(progressCache + kStuntProgressCaughtCountOffset);
    if (afterStuntCount <= beforeStuntCount) {
        return;
    }

    const int maxStuntCount =
        *reinterpret_cast<int*>(progressCache + kStuntProgressMaxCountOffset);
    const int safeAfterStuntCount =
        maxStuntCount > 0 && afterStuntCount > maxStuntCount
            ? maxStuntCount
            : afterStuntCount;

    for (int starIndex = beforeStuntCount; starIndex < safeAfterStuntCount; ++starIndex) {
        const int starId = progressCache[kStuntProgressCaughtIdsOffset + starIndex];
        EmitStuntArenaStarCaughtEvent(starId);
    }
}

void Hook_Engine_UpdateRaceProgress() {
    const int trackIndex = GetCurrentTrackIndex();
    const uint32_t beforeFlags = GetTrackProgressFlags(trackIndex);

    Orig_Engine_UpdateRaceProgress();

    const uint32_t afterFlags = GetTrackProgressFlags(trackIndex);
    EmitNewTrackFlagEvents(
        trackIndex,
        beforeFlags,
        afterFlags,
        TRACKPROGRESS_RACE_WON,
        ProgressEventKind::SingleRaceWon
    );
}

void Hook_Cup_OnStageFinished(uint64_t param1, uint64_t param2, uint64_t param3, FILE* file) {
    std::array<uint32_t, kStockProgressTrackCount> beforeFlags = {};

    for (int trackIndex = 0; trackIndex < kStockProgressTrackCount; ++trackIndex) {
        beforeFlags[trackIndex] = GetTrackProgressFlags(trackIndex);
    }

    Orig_Cup_OnStageFinished(param1, param2, param3, file);

    std::array<bool, 6> emittedTiers = {};
    for (int trackIndex = 0; trackIndex < kStockProgressTrackCount; ++trackIndex) {
        TrackInfo* track = GetTrackInfoByRuntimeIndex(trackIndex);
        if (track == nullptr) {
            continue;
        }

        const uint32_t afterFlags = static_cast<uint32_t>(track->trackProgressFlags);
        const bool championshipFlagWasSet =
            (beforeFlags[trackIndex] & TRACKPROGRESS_COMPLETED) == 0 &&
            (afterFlags & TRACKPROGRESS_COMPLETED) != 0;

        if (!championshipFlagWasSet) {
            continue;
        }

        const int difficultyTier = track->difficultyRating;
        if (difficultyTier < 0 || difficultyTier >= static_cast<int>(emittedTiers.size())) {
            EmitChampionshipWonEvent(difficultyTier);
            continue;
        }

        if (!emittedTiers[difficultyTier]) {
            emittedTiers[difficultyTier] = true;
            EmitChampionshipWonEvent(difficultyTier);
        }
    }
}

} // namespace Randomizer
