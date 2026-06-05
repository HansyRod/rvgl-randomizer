#include "ThirtyCarCupMod.h"
#include "30CarMod.h"
#include "Addresses.h"
#include "CupHooks.h"
#include "Logger.h"
#include "RaceInitHooks.h"
#include "RandomizerState.h"
#include "RVGLFunctions.h"
#include "TrackHooks.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <random>
#include <unordered_map>
#include <vector>

namespace Randomizer {

FnCup_GenerateOpponentGrid Orig_Cup_GenerateOpponentGrid = nullptr;
FnBuildGrid Orig_BuildGrid = nullptr;
FnUpdateCupPostRaceProgress Orig_UpdateCupPostRaceProgress = nullptr;
FnDrawCupProgressMessage Orig_DrawCupProgressMessage = nullptr;

namespace {

constexpr int kMaxCupCars = randomizerMaxCarCount;
constexpr int kNativeCupCars = vanillaMaxCarCount;
constexpr int kCpuRaceCarState = 3;
constexpr int kPlayerRaceCarState = 1;
constexpr int kGridCols = 5;
constexpr int kGridRows = 6;
constexpr float kColumnSpacing = 150.0f;
constexpr float kRowSpacing = 150.0f;
constexpr size_t kNativeCupRuntimeResetBytes = 0x12A8;

constexpr size_t kRaceSettingsSelectedCupOffset = 0x04;
constexpr size_t kRaceSettingsPlayerNameOffset = 0x28;
constexpr size_t kRaceSettingsPlayerModelOffset = 0x68;
constexpr size_t kRaceSettingsPlayerSkinOffset = 0x78;
constexpr size_t kRaceSettingsPickupsOffset = 0x9E;

constexpr size_t kGameModeTrackOffset = 0x08;
constexpr size_t kGameModeCountdownOffset = 0x14;
constexpr size_t kGameModeLapsOffset = 0x18;
constexpr size_t kGameModeReverseOffset = 0x30;
constexpr size_t kGameModeMirrorOffset = 0x31;
constexpr size_t kGameModePickupsOffset = 0x32;

constexpr size_t kPlayerRaceInfoParticipantCountOffset = 0x04;
constexpr size_t kPlayerRaceInfoModeOffset = 0x0C;
constexpr size_t kPlayerRaceInfoLapsOffset = 0x10;
constexpr size_t kPlayerRaceInfoMirrorOffset = 0x14;
constexpr size_t kPlayerRaceInfoReverseOffset = 0x18;
constexpr size_t kPlayerRaceInfoPickupsOffset = 0x20;
constexpr size_t kPlayerRaceInfoCountdownOffset = 0x28;
constexpr size_t kPlayerRaceInfoUnknown34Offset = 0x34;
constexpr size_t kPlayerRaceInfoTrackFolderOffset = 0x40;

struct ThirtyCarCupState {
    bool active = false;
    bool rosterGenerated = false;
    int selectedCupIndex = -1;
    int lastRecordedStage = -1;
    bool gridApplied = false;
    bool playerMovedToBack = false;
    bool pendingPointTimerInitialized = false;
    std::chrono::steady_clock::time_point pendingPointNextTick = {};
    CupProfile* activeCup = nullptr;
    const RandomizedCup* cupConfig = nullptr;
    std::array<CupParticipantEntry, kMaxCupCars> participants = {};
    std::array<CupParticipantEntry, kMaxCupCars> standings = {};
    std::array<int, kMaxCupCars> runtimeCarIds = {};
    std::vector<int> pointsTable;

    void Reset() {
        active = false;
        rosterGenerated = false;
        selectedCupIndex = -1;
        lastRecordedStage = -1;
        gridApplied = false;
        playerMovedToBack = false;
        pendingPointTimerInitialized = false;
        pendingPointNextTick = {};
        activeCup = nullptr;
        cupConfig = nullptr;
        pointsTable.clear();
        participants = {};
        standings = {};
        runtimeCarIds.fill(-1);
    }
};

ThirtyCarCupState g_cupState;

struct NativePendingSnapshot {
    std::array<int, kNativeCupCars> pendingPoints = {};
};

enum class PendingPointAdvance {
    None,
    Step,
    Flush
};

using FnResetCurrentTrackSelectionState = void(*)();
using FnRaceTeardownAndSave = void(*)();
using FnLevelDestroyAndFree = void(*)();
using FnUnknownGameState = void(*)(uint64_t param1, const char* cupName, uint64_t param3, FILE* file);

FnResetCurrentTrackSelectionState RVGL_ResetCurrentTrackSelectionState =
    reinterpret_cast<FnResetCurrentTrackSelectionState>(AbsFromRva(RVA_RESET_CURRENT_TRACK_SELECTION_STATE));
FnRaceTeardownAndSave RVGL_RaceTeardownAndSave =
    reinterpret_cast<FnRaceTeardownAndSave>(AbsFromRva(RVA_RACE_TEARDOWN_AND_SAVE));
FnLevelDestroyAndFree RVGL_LevelDestroyAndFree =
    reinterpret_cast<FnLevelDestroyAndFree>(AbsFromRva(RVA_LEVEL_DESTROY_AND_FREE));
FnUnknownGameState RVGL_UnknownGameState =
    reinterpret_cast<FnUnknownGameState>(AbsFromRva(RVA_LOAD_NEXT_RACE_FROM_PLAYER_RACE_INFO));

uint8_t* GetRaceSettings() {
    return *reinterpret_cast<uint8_t**>(AbsFromRva(RVA_RACE_SETTINGS_PTR));
}

uint8_t* GetPlayerRaceInfo() {
    return *reinterpret_cast<uint8_t**>(AbsFromRva(RVA_PLAYER_RACE_INFO_PTR));
}

CupProfile*& ActiveCupRef() {
    return *reinterpret_cast<CupProfile**>(AbsFromRva(RVA_ACTIVE_CUP_PTR));
}

CupResultRuntime& CupResult() {
    return *reinterpret_cast<CupResultRuntime*>(AbsFromRva(RVA_CUP_RESULT));
}

int& CurrentCupIndex() {
    return *reinterpret_cast<int*>(AbsFromRva(RVA_CURRENT_CUP_INDEX));
}

int& CurrentCupStageIndex() {
    return *reinterpret_cast<int*>(AbsFromRva(RVA_CURRENT_CUP_STAGE_INDEX));
}

int& CupTriesLeft() {
    return *reinterpret_cast<int*>(AbsFromRva(RVA_CUP_TRIES_LEFT));
}

int& CupStageDirection() {
    return *reinterpret_cast<int*>(AbsFromRva(RVA_CUP_STAGE_DIRECTION));
}

int& CupPostRaceState() {
    return *reinterpret_cast<int*>(AbsFromRva(RVA_CUP_POST_RACE_STATE));
}

int& NativeParticipantCount() {
    return *reinterpret_cast<int*>(AbsFromRva(RVA_RACE_PARTICIPANT_COUNT));
}

CupParticipantEntry* NativeCupParticipants() {
    return reinterpret_cast<CupParticipantEntry*>(AbsFromRva(RVA_NATIVE_CUP_PARTICIPANTS));
}

CupParticipantEntry* NativeCupStandings() {
    return reinterpret_cast<CupParticipantEntry*>(AbsFromRva(RVA_NATIVE_CUP_STANDINGS_SORTED));
}

CarInfo* GetCarInfoTable() {
    return *reinterpret_cast<CarInfo**>(AbsFromRva(RVA_CAR_TABLE));
}

const CarInfo* GetCarInfoByModelId(int modelId) {
    CarInfo* cars = GetCarInfoTable();
    if (cars == nullptr || modelId < 0 || modelId >= GetTotalCarModelCount()) {
        return nullptr;
    }

    return &cars[modelId];
}

CarEntityRuntime* GetLiveCarById(int runtimeCarId) {
    CarEntityRuntime* car = *reinterpret_cast<CarEntityRuntime**>(AbsFromRva(RVA_CAR_LIST_HEAD));
    int visited = 0;

    while (car != nullptr && visited < 64) {
        if (car->nCarArrayIndex == runtimeCarId) {
            return car;
        }

        car = car->pNext;
        ++visited;
    }

    return nullptr;
}

char* MutableText(const char* text) {
    return const_cast<char*>(text != nullptr ? text : "");
}

char** GetLocaleStrings() {
    return *reinterpret_cast<char***>(AbsFromRva(RVA_LOCALE_STRINGS_PTR));
}

const char* LocaleString(int index, const char* fallback) {
    char** strings = GetLocaleStrings();
    if (strings == nullptr || strings[index] == nullptr) {
        return fallback;
    }

    return strings[index];
}

int GetSelectedCupIndexFromSettings() {
    uint8_t* settings = GetRaceSettings();
    return settings != nullptr ? *reinterpret_cast<int*>(settings + kRaceSettingsSelectedCupOffset) : -1;
}

int GetSelectedPlayerModelFromSettings() {
    uint8_t* settings = GetRaceSettings();
    return settings != nullptr ? *reinterpret_cast<int*>(settings + kRaceSettingsPlayerModelOffset) : 0;
}

int GetSelectedPlayerSkinFromSettings() {
    uint8_t* settings = GetRaceSettings();
    return settings != nullptr ? *reinterpret_cast<int*>(settings + kRaceSettingsPlayerSkinOffset) : 0;
}

char* GetPlayerNameFromSettings() {
    uint8_t* settings = GetRaceSettings();
    return settings != nullptr ? reinterpret_cast<char*>(settings + kRaceSettingsPlayerNameOffset) : MutableText("Player");
}

CupProfile* ResolveActiveCupFromSelection(int selectedCupIndex) {
    return GetCupProfileByCupID(selectedCupIndex);
}

const RandomizedCup* FindCupConfig(int selectedCupIndex) {
    return GetCupConfigByCupID(selectedCupIndex);
}

bool IsThirtyCarCup(CupProfile* cup) {
    return cup != nullptr && cup->numCars > kNativeCupCars && cup->numCars <= kMaxCupCars;
}

bool IsCupDCEnabled() {
    return *reinterpret_cast<bool*>(AbsFromRva(RVA_CUP_DC));
}

bool IsRandomCarColorEnabled() {
    return *reinterpret_cast<bool*>(AbsFromRva(RVA_RANDOM_CAR_COLORS_ENABLED));
}

bool IsCupProgressTableVisible() {
    int* displayState = *reinterpret_cast<int**>(AbsFromRva(RVA_POST_RACE_MENU_DISPLAY_STATE_PTR));
    return displayState != nullptr && *displayState == 4;
}

float GetUiViewportCenterX() {
    constexpr float nativeUiWidth = 640.0f;
    constexpr float nativeUiHalfWidth = nativeUiWidth * 0.5f;

    uintptr_t viewportPtrSlot = *reinterpret_cast<uintptr_t*>(AbsFromRva(RVA_UI_VIEWPORT_PTR_PTR));
    if (viewportPtrSlot == 0) {
        return nativeUiHalfWidth;
    }

    uintptr_t viewport = *reinterpret_cast<uintptr_t*>(viewportPtrSlot);
    if (viewport == 0) {
        return nativeUiHalfWidth;
    }

    const float viewportCenterX = *reinterpret_cast<float*>(viewport + 0x10);
    if (!std::isfinite(viewportCenterX) || viewportCenterX <= 0.0f) {
        return nativeUiHalfWidth;
    }

    return viewportCenterX;
}

float GetCenteredPanelX(float panelWidth) {
    constexpr float nativeUiWidth = 640.0f;
    constexpr float nativeUiHalfWidth = nativeUiWidth * 0.5f;

    const float uiScale = *reinterpret_cast<float*>(AbsFromRva(RVA_CUP_PROGRESS_UI_COORD_SCALE));
    const float scale = std::isfinite(uiScale) && uiScale > 0.0f ? uiScale : 0.5f;
    return (nativeUiWidth - panelWidth) * scale + GetUiViewportCenterX() * scale - nativeUiHalfWidth;
}

std::vector<int> BuildCpuModelPool(
    int rating,
    const std::unordered_map<int, bool>& usedModels
) {
    std::vector<int> pool;

    const int totalModels = GetTotalCarModelCount();
    for (int modelId = 0; modelId < totalModels; ++modelId) {
        if (!IsCarModelCpuSelectable(modelId)) {
            continue;
        }

        if (GetCarModelRating(modelId) != rating) {
            continue;
        }

        if (usedModels.find(modelId) != usedModels.end()) {
            continue;
        }

        pool.push_back(modelId);
    }

    return pool;
}

int PickCupOpponentModel(
    int rating,
    int playerModelId,
    std::unordered_map<int, bool>& usedModels
) {
    std::vector<int> pool = BuildCpuModelPool(rating, usedModels);
    if (!pool.empty()) {
        const int modelId = PickRandomFromPool(pool);
        usedModels[modelId] = true;
        return modelId;
    }

    pool.clear();
    const int totalModels = GetTotalCarModelCount();
    for (int modelId = 0; modelId < totalModels; ++modelId) {
        if (IsCarModelCpuSelectable(modelId) &&
            GetCarModelRating(modelId) == rating) {
            pool.push_back(modelId);
        }
    }

    if (!pool.empty()) {
        return PickRandomFromPool(pool);
    }

    return playerModelId >= 0 ? playerModelId : 0;
}

int PickCpuSkinForCup(int modelId) {
    if (!IsRandomCarColorEnabled()) {
        return 0;
    }

    const CarInfo* car = GetCarInfoByModelId(modelId);
    if (car == nullptr || car->skinCount <= 1) {
        return 0;
    }

    static std::mt19937 rng{ std::random_device{}() };
    std::uniform_int_distribution<int> dist(0, car->skinCount - 1);
    return dist(rng);
}

void BuildParticipantName(int modelId, char (&outName)[16]) {
    const CarInfo* car = GetCarInfoByModelId(modelId);
    if (car == nullptr || car->displayName[0] == '\0') {
        std::snprintf(outName, sizeof(outName), "Car %02d", modelId);
        return;
    }

    std::snprintf(outName, sizeof(outName), "%.*s", 15, car->displayName);
}

void MirrorNativeCupTables() {
    const int count = (std::min)(g_cupState.activeCup != nullptr ? g_cupState.activeCup->numCars : 0, kNativeCupCars);
    CupParticipantEntry* nativeParticipants = NativeCupParticipants();
    CupParticipantEntry* nativeStandings = NativeCupStandings();

    for (int i = 0; i < count; ++i) {
        nativeParticipants[i] = g_cupState.participants[i];
        nativeStandings[i] = g_cupState.standings[i];
    }
}

void ResetNativeCupRuntimeState() {
    std::memset(
        reinterpret_cast<void*>(AbsFromRva(RVA_CURRENT_CUP_INDEX)),
        0,
        kNativeCupRuntimeResetBytes
    );
}

int GetPointsForPosition(int zeroBasedPosition) {
    if (zeroBasedPosition >= 0 && zeroBasedPosition < static_cast<int>(g_cupState.pointsTable.size())) {
        return g_cupState.pointsTable[zeroBasedPosition];
    }

    if (g_cupState.activeCup != nullptr &&
        zeroBasedPosition >= 0 &&
        zeroBasedPosition < kNativeCupCars) {
        return g_cupState.activeCup->pointsTable[zeroBasedPosition];
    }

    return 0;
}

void SortCupStandings() {
    const int count = std::clamp(g_cupState.activeCup->numCars, 0, kMaxCupCars);
    std::copy(
        g_cupState.participants.begin(),
        g_cupState.participants.begin() + count,
        g_cupState.standings.begin()
    );

    const int stage = std::clamp(CurrentCupStageIndex(), 0, 15);
    std::stable_sort(
        g_cupState.standings.begin(),
        g_cupState.standings.begin() + count,
        [stage](const CupParticipantEntry& lhs, const CupParticipantEntry& rhs) {
            if (lhs.totalPoints != rhs.totalPoints) {
                return lhs.totalPoints > rhs.totalPoints;
            }

            return lhs.finishPositionByStage[stage] < rhs.finishPositionByStage[stage];
        }
    );
}

void UpdateCupResultFromStandings() {
    if (!g_cupState.active || g_cupState.activeCup == nullptr) {
        return;
    }

    const int count = std::clamp(g_cupState.activeCup->numCars, 0, kMaxCupCars);
    CupResultRuntime& result = CupResult();

    result.playerOverallRank = count;
    for (int i = 0; i < count; ++i) {
        if (g_cupState.standings[i].participantIndex == 0) {
            result.playerOverallRank = i + 1;
            break;
        }
    }

    for (int i = 0; i < 3 && i < count; ++i) {
        result.standingsSnapshot[i] = g_cupState.standings[i].modelId;
    }
    result.playerFinalRank = g_cupState.participants[0].modelId;
}

void RecordCurrentStageResultsOnce() {
    if (!g_cupState.active || g_cupState.activeCup == nullptr) {
        return;
    }

    const bool raceFinished = *reinterpret_cast<bool*>(AbsFromRva(RVA_RACE_FINISHED_FLAG));
    if (!raceFinished) {
        return;
    }

    const int stage = CurrentCupStageIndex();
    if (stage < 0 || stage >= 16 || g_cupState.lastRecordedStage == stage) {
        return;
    }

    const int count = std::clamp(g_cupState.activeCup->numCars, 0, kMaxCupCars);
    for (int participantIndex = 0; participantIndex < count; ++participantIndex) {
        CarEntityRuntime* car = GetLiveCarById(participantIndex);

        int finishPosition = participantIndex;
        int finishTime = 0;
        if (car != nullptr) {
            finishPosition = car->finishPosition >= 0 ? car->finishPosition : car->racePositionIndex;
            finishTime = car->finishTimeMs;
        }

        finishPosition = std::clamp(finishPosition, 0, count - 1);
        CupParticipantEntry& participant = g_cupState.participants[participantIndex];
        participant.finishPositionByStage[stage] = finishPosition;
        participant.finishTimeByStage[stage] = finishTime;
        participant.pendingPoints = GetPointsForPosition(finishPosition);
    }

    g_cupState.lastRecordedStage = stage;
    SortCupStandings();
    UpdateCupResultFromStandings();
    MirrorNativeCupTables();

    Logger::TimestampLogf(
        "[ThirtyCarCupMod] Recorded stage %d results for %d-car cup",
        stage,
        count
    );
}

void ApplyPendingPointAdvance(PendingPointAdvance advance) {
    if (advance == PendingPointAdvance::None || g_cupState.activeCup == nullptr) {
        return;
    }

    const int count = std::clamp(g_cupState.activeCup->numCars, 0, kMaxCupCars);
    for (int i = 0; i < count; ++i) {
        CupParticipantEntry& participant = g_cupState.participants[i];
        if (participant.pendingPoints == 0) {
            continue;
        }

        if (advance == PendingPointAdvance::Flush) {
            participant.totalPoints += participant.pendingPoints;
            participant.pendingPoints = 0;
            continue;
        }

        const int step = participant.pendingPoints > 0 ? 1 : -1;
        participant.totalPoints += step;
        participant.pendingPoints -= step;
    }
}

bool HasPendingPoints() {
    if (g_cupState.activeCup == nullptr) {
        return false;
    }

    const int count = std::clamp(g_cupState.activeCup->numCars, 0, kMaxCupCars);
    return std::any_of(
        g_cupState.participants.begin(),
        g_cupState.participants.begin() + count,
        [](const CupParticipantEntry& participant) {
            return participant.pendingPoints != 0;
        }
    );
}

NativePendingSnapshot CaptureNativePendingSnapshot() {
    NativePendingSnapshot snapshot;
    CupParticipantEntry* nativeParticipants = NativeCupParticipants();
    const int count = std::clamp(g_cupState.activeCup != nullptr ? g_cupState.activeCup->numCars : 0, 0, kNativeCupCars);

    for (int i = 0; i < count; ++i) {
        snapshot.pendingPoints[i] = nativeParticipants[i].pendingPoints;
    }

    return snapshot;
}

bool DidNativeFlushPendingPoints(const NativePendingSnapshot& before) {
    CupParticipantEntry* nativeParticipants = NativeCupParticipants();
    const int count = std::clamp(g_cupState.activeCup != nullptr ? g_cupState.activeCup->numCars : 0, 0, kNativeCupCars);
    bool hadPending = false;
    bool flushedMoreThanOnePoint = false;

    for (int i = 0; i < count; ++i) {
        const int pendingBefore = before.pendingPoints[i];
        if (pendingBefore == 0) {
            continue;
        }

        hadPending = true;
        if (nativeParticipants[i].pendingPoints != 0) {
            return false;
        }
        if (std::abs(pendingBefore) > 1) {
            flushedMoreThanOnePoint = true;
        }
    }

    return hadPending && flushedMoreThanOnePoint;
}

void ResetPendingPointTimer() {
    g_cupState.pendingPointTimerInitialized = false;
    g_cupState.pendingPointNextTick = {};
}

bool AdvancePendingPointsOnTimer() {
    constexpr auto pendingPointInitialDelay = std::chrono::milliseconds(2000);
    constexpr auto pendingPointTick = std::chrono::milliseconds(200);

    if (CupPostRaceState() != 4 || !HasPendingPoints()) {
        ResetPendingPointTimer();
        return false;
    }

    const auto now = std::chrono::steady_clock::now();
    if (!g_cupState.pendingPointTimerInitialized) {
        g_cupState.pendingPointTimerInitialized = true;
        g_cupState.pendingPointNextTick = now + pendingPointInitialDelay;
        return false;
    }

    bool advanced = false;
    while (now >= g_cupState.pendingPointNextTick && HasPendingPoints()) {
        ApplyPendingPointAdvance(PendingPointAdvance::Step);
        g_cupState.pendingPointNextTick += pendingPointTick;
        advanced = true;
    }

    return advanced;
}

void FlushAllPendingPoints() {
    if (g_cupState.activeCup == nullptr) {
        return;
    }

    const int count = std::clamp(g_cupState.activeCup->numCars, 0, kMaxCupCars);
    for (int i = 0; i < count; ++i) {
        CupParticipantEntry& participant = g_cupState.participants[i];
        participant.totalPoints += participant.pendingPoints;
        participant.pendingPoints = 0;
    }
}

class NativeCupClamp {
public:
    explicit NativeCupClamp(CupProfile* cup)
        : cup_(cup),
          savedNumCars_(cup != nullptr ? cup->numCars : 0),
          savedParticipantCount_(NativeParticipantCount()) {
        if (cup_ != nullptr && cup_->numCars > kNativeCupCars) {
            cup_->numCars = kNativeCupCars;
        }
        if (NativeParticipantCount() > kNativeCupCars) {
            NativeParticipantCount() = kNativeCupCars;
        }
    }

    ~NativeCupClamp() {
        if (cup_ != nullptr) {
            cup_->numCars = savedNumCars_;
        }
        NativeParticipantCount() = savedParticipantCount_;
    }

private:
    CupProfile* cup_;
    int savedNumCars_;
    int savedParticipantCount_;
};

void DrawSizedText(
    float x,
    float y,
    float width,
    float height,
    uint32_t color,
    const char* text,
    float maxWidth = 0.0f
) {
    RVGL_DrawUIText(x, y, width, height, color, MutableText(text), maxWidth, 0);
}

void DrawText(float x, float y, uint32_t color, const char* text, float maxWidth = 0.0f) {
    DrawSizedText(x, y, 6.5f, 10.0f, color, text, maxWidth);
}

void DrawRightText(float rightX, float y, uint32_t color, const char* text) {
    const int64_t len = RVGL_UTF8GetVisibleCharCount(MutableText(text));
    DrawText(rightX - static_cast<float>(len) * 6.5f, y, color, text);
}

void DrawRightSizedText(
    float rightX,
    float y,
    float width,
    float height,
    uint32_t color,
    const char* text
) {
    const int64_t len = RVGL_UTF8GetVisibleCharCount(MutableText(text));
    DrawSizedText(rightX - static_cast<float>(len) * width, y, width, height, color, text);
}

const char* GetFinishSuffix(int zeroBasedPosition) {
    const int oneBasedPosition = zeroBasedPosition + 1;
    const int lastTwoDigits = oneBasedPosition % 100;
    if (lastTwoDigits >= 11 && lastTwoDigits <= 13) {
        return LocaleString(0x122, "th");
    }

    const int lastDigit = oneBasedPosition % 10;
    if (lastDigit == 1) {
        return LocaleString(0x11f, "st");
    }
    if (lastDigit == 2) {
        return LocaleString(0x120, "nd");
    }
    if (lastDigit == 3) {
        return LocaleString(0x121, "rd");
    }

    return LocaleString(0x122, "th");
}

void FormatStageFinish(const CupParticipantEntry& standing, int stage, char (&outText)[16]) {
    const int finishPosition = standing.finishPositionByStage[stage];
    std::snprintf(
        outText,
        sizeof(outText),
        "%d%s",
        finishPosition + 1,
        GetFinishSuffix(finishPosition)
    );
}

void FormatStandingName(const CupParticipantEntry& standing, char (&outName)[20]) {
    if (standing.participantIndex == 0) {
        std::snprintf(outName, sizeof(outName), "%.*s", 19, GetPlayerNameFromSettings());
        return;
    }

    const CarInfo* car = GetCarInfoByModelId(standing.modelId);
    if (car == nullptr || car->displayName[0] == '\0') {
        std::snprintf(outName, sizeof(outName), "Car %02d", standing.modelId);
        return;
    }

    std::snprintf(outName, sizeof(outName), "%.*s", 19, car->displayName);
}

void DrawThirtyCarCupTable() {
    if (!g_cupState.active || g_cupState.activeCup == nullptr) {
        return;
    }

    const int count = std::clamp(g_cupState.activeCup->numCars, 0, kMaxCupCars);
    if (count <= kNativeCupCars) {
        return;
    }

    if (!IsCupProgressTableVisible()) {
        return;
    }

    constexpr int maxStageColumns = 5;

    const int currentStage = std::clamp(CurrentCupStageIndex(), 0, 15);
    const int visibleStageCount = std::clamp(currentStage + 1, 1, maxStageColumns);
    const int firstVisibleStage = currentStage - visibleStageCount + 1;

    const float textWidth = count > 24 ? 6.2f : (count > 20 ? 7.5f : 8.0f);
    const float textHeight = count > 24 ? 11.0f : (count > 20 ? 14.0f : 16.0f);
    const float rowHeight = textHeight;

    const float panelY = 70.0f;
    const float contentPaddingY = 4.0f;
    const float headerY = panelY + contentPaddingY;
    const float rowStartY = headerY + rowHeight + 1.0f;
    const float nameMaxWidth = 118.0f;
    const float stageSpacing = count > 24 ? 30.0f : (count > 20 ? 35.0f : 40.0f);
    const float contentPaddingX = 4.0f;
    const float nameToStageGap = count > 24 ? 8.0f : (count > 20 ? 12.0f : 16.0f);
    const float stageColumnRightInset = 20.0f;
    const float pointsGap = 20.0f;
    const float pendingGap = 28.0f;
    const float stageStartFromTable = nameMaxWidth + nameToStageGap;
    const float pointsRightFromTable =
        stageStartFromTable +
        static_cast<float>(visibleStageCount) * stageSpacing +
        pointsGap;
    const float pendingRightFromTable = pointsRightFromTable + pendingGap;
    const float panelWidth = pendingRightFromTable + contentPaddingX * 2.0f;
    const float panelX = GetCenteredPanelX(panelWidth);
    const float tableX = panelX + contentPaddingX;
    const float stageStartX = tableX + stageStartFromTable;
    const float pointsRightX = tableX + pointsRightFromTable;
    const float pendingRightX = tableX + pendingRightFromTable;
    const float panelHeight = std::clamp(
        rowStartY - panelY + static_cast<float>(count) * rowHeight + contentPaddingY,
        180.0f,
        362.0f
    );

    RVGL_UIDrawRoundedRect(panelX, panelY, panelWidth, panelHeight, 0, 0, 0xb0181818, 0xff, 1);
    RVGL_FlushDeferredUIBatches();
    RVGL_SetupGLRenderState();

    DrawSizedText(
        tableX,
        headerY,
        textWidth,
        textHeight,
        0xff00ffff,
        LocaleString(0x115, "Standings"),
        nameMaxWidth
    );

    for (int stageColumn = 0; stageColumn < visibleStageCount; ++stageColumn) {
        char stageLabel[8] = {};
        std::snprintf(stageLabel, sizeof(stageLabel), "%d", firstVisibleStage + stageColumn + 1);
        DrawRightSizedText(
            stageStartX + static_cast<float>(stageColumn) * stageSpacing + stageColumnRightInset,
            headerY,
            textWidth,
            textHeight,
            0xff00ffff,
            stageLabel
        );
    }
    DrawRightSizedText(pointsRightX, headerY, textWidth, textHeight, 0xff00ffff, LocaleString(0x129, "Pts"));

    for (int row = 0; row < count; ++row) {
        const CupParticipantEntry& standing = g_cupState.standings[row];
        const float y = rowStartY + static_cast<float>(row) * rowHeight;

        char name[20] = {};
        FormatStandingName(standing, name);
        DrawSizedText(tableX, y, textWidth, textHeight, 0xffffffff, name, nameMaxWidth);

        for (int stageColumn = 0; stageColumn < visibleStageCount; ++stageColumn) {
            char finishText[16] = {};
            FormatStageFinish(standing, firstVisibleStage + stageColumn, finishText);
            DrawRightSizedText(
                stageStartX + static_cast<float>(stageColumn) * stageSpacing + stageColumnRightInset,
                y,
                textWidth,
                textHeight,
                0xff00ff00,
                finishText
            );
        }

        char points[16] = {};
        std::snprintf(points, sizeof(points), "%2.2d", standing.totalPoints);
        DrawRightSizedText(pointsRightX, y, textWidth, textHeight, 0xffffff00, points);

        if (standing.pendingPoints != 0) {
            char pending[16] = {};
            std::snprintf(pending, sizeof(pending), "%+d", standing.pendingPoints);
            DrawRightSizedText(pendingRightX, y, textWidth, textHeight, 0xffff0000, pending);
        }
    }
}

} // anonymous namespace

bool IsThirtyCarCupActive() {
    return g_cupState.active && IsThirtyCarCup(g_cupState.activeCup);
}

void ResetThirtyCarCupState() {
    g_cupState.Reset();
}

void Hook_Cup_GenerateOpponentGrid() {
    const int selectedCupIndex = GetSelectedCupIndexFromSettings();
    CupProfile* cup = ResolveActiveCupFromSelection(selectedCupIndex);
    if (!IsThirtyCarCup(cup)) {
        ResetThirtyCarCupState();
        Orig_Cup_GenerateOpponentGrid();
        return;
    }

    g_cupState.Reset();
    ResetNativeCupRuntimeState();
    g_cupState.active = true;
    g_cupState.rosterGenerated = true;
    g_cupState.selectedCupIndex = selectedCupIndex;
    g_cupState.activeCup = cup;
    g_cupState.cupConfig = FindCupConfig(selectedCupIndex);
    ActiveCupRef() = cup;
    CurrentCupIndex() = selectedCupIndex;
    CupTriesLeft() = cup->numTries;

    if (g_cupState.cupConfig != nullptr && !g_cupState.cupConfig->pointsTable.empty()) {
        g_cupState.pointsTable = g_cupState.cupConfig->pointsTable;
    }
    else {
        g_cupState.pointsTable.assign(cup->pointsTable, cup->pointsTable + kNativeCupCars);
    }

    const int count = std::clamp(cup->numCars, kNativeCupCars + 1, kMaxCupCars);
    const int playerModelId = GetSelectedPlayerModelFromSettings();
    const int playerSkinId = GetSelectedPlayerSkinFromSettings();
    std::unordered_map<int, bool> usedModels;

    g_cupState.participants[0].participantIndex = 0;
    g_cupState.participants[0].modelId = playerModelId;
    g_cupState.participants[0].skinId = playerSkinId;
    usedModels[playerModelId] = true;

    std::vector<int> requestedRatings;
    const int classCounts[6] = {
        cup->maxRookie,
        cup->maxAmateur,
        cup->maxAdvanced,
        cup->maxSemiPro,
        cup->maxPro,
        cup->maxSuperPro
    };
    for (int rating = 0; rating < 6; ++rating) {
        for (int i = 0; i < classCounts[rating]; ++i) {
            requestedRatings.push_back(rating);
        }
    }

    while (static_cast<int>(requestedRatings.size()) < count - 1) {
        requestedRatings.push_back(GetCarModelRating(playerModelId));
    }

    for (int i = 1; i < count; ++i) {
        const int rating = requestedRatings[i - 1];
        const int modelId = PickCupOpponentModel(rating, playerModelId, usedModels);
        g_cupState.participants[i].participantIndex = i;
        g_cupState.participants[i].modelId = modelId;
        g_cupState.participants[i].skinId = PickCpuSkinForCup(modelId);
    }

    SortCupStandings();
    MirrorNativeCupTables();

    Logger::TimestampLogf(
        "[ThirtyCarCupMod] Generated %d-car cup roster for '%s'",
        count,
        cup->displayName
    );
}

void Hook_BuildGrid() {
    if (!IsThirtyCarCupActive()) {
        Orig_BuildGrid();
        return;
    }

    CupProfile* cup = g_cupState.activeCup;
    const int stageIndex = std::clamp(CurrentCupStageIndex(), 0, 15);
    const CupStage& stage = cup->stages[stageIndex];
    g_cupState.gridApplied = false;
    g_cupState.playerMovedToBack = false;
    g_cupState.runtimeCarIds.fill(-1);

    uint8_t* settings = GetRaceSettings();
    uint8_t* gameMode = reinterpret_cast<uint8_t*>(AbsFromRva(RVA_GAME_MODE));
    uint8_t* playerRaceInfo = GetPlayerRaceInfo();
    uint8_t* playerRaceInfo34Source =
        *reinterpret_cast<uint8_t**>(AbsFromRva(RVA_PLAYER_RACE_INFO_34_SOURCE_PTR));

    if (settings != nullptr) {
        *reinterpret_cast<int*>(settings) = MODE_CHAMPIONSHIP;
    }
    *reinterpret_cast<int*>(gameMode) = MODE_CHAMPIONSHIP;
    *reinterpret_cast<int*>(gameMode + kGameModeTrackOffset) = stage.trackID;

    RVGL_ResetCurrentTrackSelectionState();

    if (playerRaceInfo != nullptr) {
        *reinterpret_cast<int*>(playerRaceInfo + kPlayerRaceInfoModeOffset) = MODE_CHAMPIONSHIP;
        *reinterpret_cast<int*>(playerRaceInfo + kPlayerRaceInfoLapsOffset) = stage.numLaps;
        *reinterpret_cast<int*>(playerRaceInfo + kPlayerRaceInfoMirrorOffset) = stage.isMirror ? 1 : 0;
        *reinterpret_cast<int*>(playerRaceInfo + kPlayerRaceInfoReverseOffset) = stage.isReverse ? 1 : 0;

        if (settings != nullptr) {
            const uint8_t pickupsEnabled = settings[kRaceSettingsPickupsOffset];
            gameMode[kGameModePickupsOffset] = pickupsEnabled;
            *reinterpret_cast<int*>(playerRaceInfo + kPlayerRaceInfoPickupsOffset) = pickupsEnabled;
        }

        *reinterpret_cast<int*>(playerRaceInfo + kPlayerRaceInfoCountdownOffset) =
            *reinterpret_cast<int*>(gameMode + kGameModeCountdownOffset);
        *reinterpret_cast<int*>(playerRaceInfo + kPlayerRaceInfoUnknown34Offset) =
            playerRaceInfo34Source != nullptr ? static_cast<int>(*playerRaceInfo34Source) : 0;

        TrackInfo* track = GetTrackInfoByRuntimeIndex(stage.trackID);
        if (track != nullptr) {
            std::snprintf(
                reinterpret_cast<char*>(playerRaceInfo + kPlayerRaceInfoTrackFolderOffset),
                16,
                "%.*s",
                15,
                track->folderName
            );
        }
    }

    gameMode[kGameModeReverseOffset] = stage.isReverse ? 1 : 0;
    gameMode[kGameModeMirrorOffset] = stage.isMirror ? 1 : 0;
    *reinterpret_cast<int*>(gameMode + kGameModeLapsOffset) = stage.numLaps;

    const int count = std::clamp(cup->numCars, 0, kMaxCupCars);
    Orig_AddParticipantAndCount(
        kPlayerRaceCarState,
        0,
        g_cupState.participants[0].modelId,
        g_cupState.participants[0].skinId,
        1,
        1,
        GetPlayerNameFromSettings()
    );

    for (int i = 1; i < count; ++i) {
        char name[16] = {};
        BuildParticipantName(g_cupState.participants[i].modelId, name);
        Orig_AddParticipantAndCount(
            kCpuRaceCarState,
            i,
            g_cupState.participants[i].modelId,
            g_cupState.participants[i].skinId,
            2,
            i + 1,
            name
        );
    }

    Orig_AssignStartPositions();
}

void ApplyThirtyCarCupGrid() {
    if (!IsThirtyCarCupActive()) {
        return;
    }

    const int carCount = GetParticipantCount();
    const int targetCarCount = std::clamp(g_cupState.activeCup->numCars, 0, kMaxCupCars);
    if (carCount <= kNativeCupCars || carCount > targetCarCount) {
        return;
    }

    Vec3 center{ 0.0f, 0.0f, 0.0f };
    for (int carId = 0; carId < carCount; ++carId) {
        const Vec3 pos = GetCarPos(carId);
        center.x += pos.x;
        center.y += pos.y;
        center.z += pos.z;
    }

    center.x /= static_cast<float>(carCount);
    center.y /= static_cast<float>(carCount);
    center.z /= static_cast<float>(carCount);

    const float gridCenterCol = static_cast<float>(kGridCols - 1) / 2.0f;
    const float gridCenterRow = static_cast<float>(kGridRows - 1) / 2.0f;

    for (int gridIndex = 0; gridIndex < targetCarCount; ++gridIndex) {
        const int row = gridIndex / kGridCols;
        const int col = gridIndex % kGridCols;

        Vec3 pos;
        pos.x = center.x + (static_cast<float>(col) - gridCenterCol) * kColumnSpacing;
        pos.y = center.y;
        pos.z = center.z + (static_cast<float>(row) - gridCenterRow) * kRowSpacing;
        SetCarPos(gridIndex, pos);
        g_cupState.runtimeCarIds[gridIndex] = gridIndex;
    }

    g_cupState.gridApplied = true;
}

void MoveThirtyCarCupPlayerToBackAfterRacePositions() {
    if (!IsThirtyCarCupActive() ||
        !g_cupState.gridApplied ||
        g_cupState.playerMovedToBack ||
        g_cupState.activeCup == nullptr) {
        return;
    }

    const int targetCarCount = std::clamp(g_cupState.activeCup->numCars, 0, kMaxCupCars);
    if (MoveRuntimeCarsToBackAfterRacePositions(g_cupState.runtimeCarIds, targetCarCount)) {
        g_cupState.playerMovedToBack = true;
    }
}

void Hook_UpdateCupPostRaceProgress() {
    if (!IsThirtyCarCupActive()) {
        Orig_UpdateCupPostRaceProgress();
        return;
    }

    RecordCurrentStageResultsOnce();
    MirrorNativeCupTables();

    const int postRaceStateBefore = CupPostRaceState();
    const NativePendingSnapshot nativePendingBefore = CaptureNativePendingSnapshot();

    {
        NativeCupClamp clamp(g_cupState.activeCup);
        Orig_UpdateCupPostRaceProgress();
    }

    const int postRaceStateAfter = CupPostRaceState();
    bool pointsChanged = false;
    if (postRaceStateBefore == 4 && postRaceStateAfter != 4) {
        FlushAllPendingPoints();
        pointsChanged = true;
    }
    else if (postRaceStateBefore == 4 &&
             postRaceStateAfter == 4 &&
             DidNativeFlushPendingPoints(nativePendingBefore)) {
        FlushAllPendingPoints();
        ResetPendingPointTimer();
        pointsChanged = true;
    }
    else {
        pointsChanged = AdvancePendingPointsOnTimer();
    }

    if (pointsChanged) {
        SortCupStandings();
    }
    UpdateCupResultFromStandings();
    MirrorNativeCupTables();
}

void PrepareThirtyCarCupStageFinished() {
    if (!IsThirtyCarCupActive()) {
        return;
    }

    RecordCurrentStageResultsOnce();
    FlushAllPendingPoints();
    SortCupStandings();
    UpdateCupResultFromStandings();
    MirrorNativeCupTables();
}

bool HandleThirtyCarCupOnStageFinished(uint64_t param1, uint64_t param3, FILE* file) {
    if (!IsThirtyCarCupActive()) {
        return false;
    }

    PrepareThirtyCarCupStageFinished();

    RVGL_RaceTeardownAndSave();
    RVGL_LevelDestroyAndFree();

    const int savedTier = CurrentCupIndex();
    if (CupStageDirection() == 2) {
        ++CurrentCupStageIndex();
    }
    else {
        --CupTriesLeft();
    }

    if (g_cupState.activeCup->numStages != CurrentCupStageIndex()) {
        Hook_BuildGrid();
        RVGL_UnknownGameState(param1, g_cupState.activeCup->displayName, param3, file);
        return true;
    }

    CupResultRuntime& result = CupResult();
    result.completedFlag = 1;
    UpdateCupResultFromStandings();
    MirrorNativeCupTables();

    if (result.playerOverallRank <= g_cupState.activeCup->overallRequiredPlace && savedTier < 5) {
        const bool cupDC = IsCupDCEnabled();
        bool allAlreadyCompleted = true;

        for (int trackIndex = 0; trackIndex < 14; ++trackIndex) {
            if (!cupDC && trackIndex == 4) {
                continue;
            }

            TrackInfo* track = GetTrackInfoByRuntimeIndex(trackIndex);
            if (track == nullptr || track->difficultyRating != savedTier) {
                continue;
            }

            if ((track->trackProgressFlags & TRACKPROGRESS_COMPLETED) == 0) {
                allAlreadyCompleted = false;
                break;
            }
        }

        for (int trackIndex = 0; trackIndex < 14; ++trackIndex) {
            TrackInfo* track = GetTrackInfoByRuntimeIndex(trackIndex);
            if (track == nullptr || track->difficultyRating != savedTier) {
                continue;
            }

            track->trackProgressFlags =
                static_cast<TrackProgressFlags>(track->trackProgressFlags | TRACKPROGRESS_COMPLETED);
        }

        if (!allAlreadyCompleted) {
            bool allTierTracksCompleted = true;
            for (int trackIndex = 0; trackIndex < 14; ++trackIndex) {
                if (!cupDC && trackIndex == 4) {
                    continue;
                }

                TrackInfo* track = GetTrackInfoByRuntimeIndex(trackIndex);
                if (track == nullptr || track->difficultyRating != savedTier) {
                    continue;
                }

                if ((track->trackProgressFlags & TRACKPROGRESS_COMPLETED) == 0) {
                    allTierTracksCompleted = false;
                    break;
                }
            }

            if (allTierTracksCompleted) {
                *reinterpret_cast<int*>(AbsFromRva(RVA_TIER_UNLOCK_TRIGGER)) = 0;
            }
        }
    }

    uint8_t* needsFrontendRefresh =
        *reinterpret_cast<uint8_t**>(AbsFromRva(RVA_FRONTEND_CUP_RESULT_FLAG_PTR));
    if (needsFrontendRefresh != nullptr) {
        *needsFrontendRefresh = 1;
    }

    void** gameStateFunction = *reinterpret_cast<void***>(AbsFromRva(RVA_GAME_STATE_FUNCTION_PTR));
    void* menuInitializeFrontend = *reinterpret_cast<void**>(AbsFromRva(RVA_MENU_INITIALIZE_FRONTEND_PTR));
    if (gameStateFunction != nullptr) {
        *gameStateFunction = menuInitializeFrontend;
    }

    return true;
}

void Hook_DrawCupProgressMessage() {
    if (!IsThirtyCarCupActive()) {
        Orig_DrawCupProgressMessage();
        return;
    }

    DrawThirtyCarCupTable();
}

} // namespace Randomizer
