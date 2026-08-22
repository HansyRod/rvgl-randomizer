#include "KnockoutMode.h"
#include "30CarMod.h"
#include "Addresses.h"
#include "Logger.h"
#include "RandomizerState.h"
#include "RVGLFunctions.h"
#include "RVGLMemory.h"
#include "RVGLRaceStructs.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>

namespace Randomizer {

namespace {

constexpr int kCarStateInactive = 0;
constexpr int kCarStatePlayer = 1;
constexpr int kCarStateCpu = 3;
constexpr int kCarStateEliminated = 9;
constexpr int kCarStateGhost = 5;
constexpr int kCarStatePassiveGhost = 4;
constexpr int kCompletedLapOffset = 0x0F40;
constexpr int kPhysicsSavedPrimaryUpdateOffset = 0x458;
constexpr int kMaxCarListTraversal = 128;
constexpr int kNativeSingleRaceLabelId = 0x15;
constexpr uint32_t kDefaultPopupPrefixColor = 0xff00ffff;
constexpr uint32_t kDefaultPopupTimeColor = 0xffffff;
constexpr char kKnockoutLabel[] = "Knockout";

struct RaceResultEntry {
    CarEntityRuntime* car;
    uint32_t totalTimeMs;
    uint8_t dnfFlag;
    uint8_t pad[3];
};

struct KnockoutResultEntry {
    CarEntityRuntime* car;
    uint32_t knockoutTimeMs;
};

struct KnockoutPopupLine {
    CarEntityRuntime* car;
    int zeroBasedPosition;
    uint32_t knockoutTimeMs;
};

struct KnockoutPopupTextLine {
    char rankText[16];
    char driverName[56];
    uint32_t color;
};

std::vector<KnockoutResultEntry> g_knockoutResults;
std::vector<KnockoutPopupTextLine> g_knockoutPopupLines;
uint32_t g_knockoutPopupUntilMs = 0;

char* MutableText(const char* text) {
    return const_cast<char*>(text != nullptr ? text : "");
}

int ReadCarInt(CarEntityRuntime* car, int offset) {
    return *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(car) + offset);
}

void WriteCarInt(CarEntityRuntime* car, int offset, int value) {
    *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(car) + offset) = value;
}

int GetCompletedLapCount(CarEntityRuntime* car) {
    return ReadCarInt(car, kCompletedLapOffset);
}

bool IsCarStillRacing(CarEntityRuntime* car) {
    const int participantCount = GetParticipantCount();
    return car != nullptr &&
           car->nCarArrayIndex >= 0 &&
           car->nCarArrayIndex < participantCount &&
           car->carState != kCarStateInactive &&
           car->carState != kCarStateEliminated &&
           car->finishTimeMs == 0;
}

bool IsKnockedOutGhostModeEnabled() {
    return GetRandomizerContext().knockoutState.knockedOutGhostMode != 0;
}

struct KnockoutCarPose {
    float position[3];
    float orientation[12];
};

bool CaptureCarPose(CarEntityRuntime* car, KnockoutCarPose& outPose) {
    if (car == nullptr || car->transform.physicsBody == nullptr) {
        return false;
    }

    outPose = {};
    outPose.position[0] = car->transform.physicsBody->position.x;
    outPose.position[1] = car->transform.physicsBody->position.y;
    outPose.position[2] = car->transform.physicsBody->position.z;

    for (int i = 0; i < 9; ++i) {
        outPose.orientation[i] = car->transform.physicsBody->orientationMatrix[i];
    }

    return true;
}

void RestoreCarPose(CarEntityRuntime* car, const KnockoutCarPose& pose) {
    if (car == nullptr || car->transform.physicsBody == nullptr) {
        return;
    }

    float position[3] = {
        pose.position[0],
        pose.position[1],
        pose.position[2]
    };
    float orientation[12] = {};
    for (int i = 0; i < 9; ++i) {
        orientation[i] = pose.orientation[i];
    }

    RVGL_SetCarTransform(&car->transform, position, orientation);
    car->transform.physicsBody->velocity = { 0.0f, 0.0f, 0.0f };
}

void DisablePassiveGhostPlayback(CarEntityRuntime* car) {
    if (car == nullptr || car->physicsEntity == nullptr) {
        return;
    }

    car->physicsEntity->primaryUpdate = nullptr;
    *reinterpret_cast<void**>(
        reinterpret_cast<uint8_t*>(car->physicsEntity) + kPhysicsSavedPrimaryUpdateOffset
    ) = nullptr;
}

std::vector<CarEntityRuntime*> GetActiveRaceCars() {
    std::vector<CarEntityRuntime*> cars;
    CarEntityRuntime* car = *reinterpret_cast<CarEntityRuntime**>(AbsFromRva(RVA_CAR_LIST_HEAD));
    int visited = 0;

    while (car != nullptr && visited < kMaxCarListTraversal) {
        if (IsCarStillRacing(car)) {
            cars.push_back(car);
        }

        car = car->pNext;
        ++visited;
    }

    return cars;
}

uint32_t GetCurrentRaceClockMs() {
    int* raceClock = *reinterpret_cast<int**>(AbsFromRva(RVA_CURRENT_RACE_CLOCK_MS_PTR));
    return raceClock != nullptr ? static_cast<uint32_t>(*raceClock) : 0;
}

RaceResultEntry* GetRaceResultTable() {
    return *reinterpret_cast<RaceResultEntry**>(AbsFromRva(RVA_RACE_RESULT_TABLE_PTR));
}

void ClearNativeKnockoutRaceState() {
    RaceResultEntry* resultTable = GetRaceResultTable();
    if (resultTable != nullptr) {
        std::memset(resultTable, 0, sizeof(RaceResultEntry) * randomizerMaxCarCount);
    }

    CarEntityRuntime* car = *reinterpret_cast<CarEntityRuntime**>(AbsFromRva(RVA_CAR_LIST_HEAD));
    int visited = 0;

    while (car != nullptr && visited < kMaxCarListTraversal) {
        if (car->nCarArrayIndex >= 0 && car->nCarArrayIndex < randomizerMaxCarCount) {
            car->finishTimeMs = 0;
            car->finishPosition = 0;
            WriteCarInt(car, kCompletedLapOffset, 0);
            if (car->carState == kCarStatePassiveGhost || car->carState == kCarStateGhost) {
                RVGL_SetCarBehaviourState(
                    car,
                    car->nCarArrayIndex == 0 ? kCarStatePlayer : kCarStateCpu
                );
            }
        }

        car = car->pNext;
        ++visited;
    }
}

uint32_t GetCarPopupColor(const CarEntityRuntime* car) {
    uint32_t* colors = *reinterpret_cast<uint32_t**>(AbsFromRva(RVA_PLAYER_COLORS_PTR));
    if (colors == nullptr || car == nullptr) {
        return kDefaultPopupPrefixColor;
    }

    return colors[car->nCarArrayIndex % 16];
}

const char* GetFinishSuffix(int zeroBasedPosition) {
    char** localeStrings = GetLocaleStrings();
    if (localeStrings == nullptr) {
        return "";
    }

    const int suffixIndex = (std::min)(zeroBasedPosition, 3) + 287;
    return localeStrings[suffixIndex] != nullptr ? localeStrings[suffixIndex] : "";
}

void FormatRaceTime(uint32_t timeMs, char (&outTime)[16]) {
    std::snprintf(
        outTime,
        sizeof(outTime),
        "%02u:%02u:%03u",
        timeMs / 60000,
        (timeMs / 1000) % 60,
        timeMs % 1000
    );
}

float GetUiViewportCenterX() {
    constexpr float nativeUiWidth = 640.0f;
    constexpr float nativeUiHalfWidth = nativeUiWidth * 0.5f;

    UiViewportRuntime* viewport = GetUiViewportRuntime();
    if (viewport == nullptr || !std::isfinite(viewport->centerX) || viewport->centerX <= 0.0f) {
        return nativeUiHalfWidth;
    }

    return viewport->centerX;
}

float GetCenteredPanelX(float panelWidth) {
    constexpr float nativeUiWidth = 640.0f;
    constexpr float nativeUiHalfWidth = nativeUiWidth * 0.5f;

    const float uiScale = GetCupProgressUiCoordScale();
    const float scale = std::isfinite(uiScale) && uiScale > 0.0f ? uiScale : 0.5f;
    return (nativeUiWidth - panelWidth) * scale + GetUiViewportCenterX() * scale - nativeUiHalfWidth;
}

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

const char* LocaleString(int index, const char* fallback) {
    char** strings = GetLocaleStrings();
    if (strings == nullptr || strings[index] == nullptr) {
        return fallback;
    }

    return strings[index];
}

void PatchNativeSingleRaceLabelForKnockout() {
    char** localeStrings = GetLocaleStrings();
    if (localeStrings != nullptr) {
        localeStrings[kNativeSingleRaceLabelId] = const_cast<char*>(kKnockoutLabel);
    }
}

bool IsFrontendMenuPanelActive() {
    uint8_t* menuSlots = *reinterpret_cast<uint8_t**>(AbsFromRva(RVA_MENU_SLOTS_PTR));
    return menuSlots != nullptr && *reinterpret_cast<uint8_t**>(menuSlots) != nullptr;
}

bool IsNativeResultsTableVisible() {
    if (!IsRaceFinished()) {
        return false;
    }

    int* displayState = GetPostRaceMenuDisplayState();
    if (displayState == nullptr) {
        return false;
    }

    if (IsFrontendMenuPanelActive()) {
        return false;
    }

    return *displayState == 0 || (*displayState >= 2 && *displayState <= 4);
}

void ShowKnockoutPopup(const std::vector<KnockoutPopupLine>& lines) {
    if (lines.empty()) {
        return;
    }

    if (lines.size() == 1) {
        const KnockoutPopupLine& line = lines.front();
        char prefix[64] = {};
        char timeText[16] = {};
        FormatRaceTime(line.knockoutTimeMs, timeText);

        std::snprintf(
            prefix,
            sizeof(prefix),
            "%d%s: %s: ",
            line.zeroBasedPosition + 1,
            GetFinishSuffix(line.zeroBasedPosition),
            line.car != nullptr ? line.car->driverName : ""
        );

        RVGL_SetPostRacePopup(
            prefix,
            timeText,
            GetCarPopupColor(line.car),
            kDefaultPopupTimeColor,
            1,
            2.0f
        );
        return;
    }

    RVGL_SetPostRacePopup(nullptr, nullptr, kDefaultPopupPrefixColor, kDefaultPopupTimeColor, 2, 0.0f);
    g_knockoutPopupLines.clear();
    g_knockoutPopupLines.reserve(lines.size());

    std::vector<KnockoutPopupLine> sortedLines = lines;
    std::sort(
        sortedLines.begin(),
        sortedLines.end(),
        [](const KnockoutPopupLine& lhs, const KnockoutPopupLine& rhs) {
            return lhs.zeroBasedPosition < rhs.zeroBasedPosition;
        }
    );

    for (const KnockoutPopupLine& line : sortedLines) {
        KnockoutPopupTextLine textLine = {};
        std::snprintf(
            textLine.rankText,
            sizeof(textLine.rankText),
            "%d%s:",
            line.zeroBasedPosition + 1,
            GetFinishSuffix(line.zeroBasedPosition)
        );
        std::snprintf(
            textLine.driverName,
            sizeof(textLine.driverName),
            "%s",
            line.car != nullptr ? line.car->driverName : ""
        );
        textLine.color = GetCarPopupColor(line.car) | 0xff000000;
        g_knockoutPopupLines.push_back(textLine);
    }

    const uint32_t currentRaceClock = GetCurrentRaceClockMs();
    const uint32_t durationMs = static_cast<uint32_t>(std::clamp(
        2000 + static_cast<int>(lines.size() - 1) * 400,
        2000,
        6000
    ));
    g_knockoutPopupUntilMs = currentRaceClock + durationMs;
}

void RewriteKnockoutResultTable() {
    RaceResultEntry* resultTable = GetRaceResultTable();
    if (resultTable == nullptr) {
        return;
    }

    const int participantCount = std::clamp(GetParticipantCount(), 0, randomizerMaxCarCount);
    if (participantCount <= 0) {
        return;
    }

    RaceResultEntry preservedFinished[randomizerMaxCarCount] = {};
    int preservedCount = 0;
    for (int i = 0; i < randomizerMaxCarCount; ++i) {
        RaceResultEntry entry = resultTable[i];
        if (entry.car == nullptr || entry.totalTimeMs == 0) {
            continue;
        }

        const auto knockoutIt = std::find_if(
            g_knockoutResults.begin(),
            g_knockoutResults.end(),
            [entry](const KnockoutResultEntry& knockoutEntry) {
                return knockoutEntry.car == entry.car;
            }
        );
        if (knockoutIt != g_knockoutResults.end()) {
            continue;
        }

        if (preservedCount < randomizerMaxCarCount) {
            preservedFinished[preservedCount++] = entry;
        }
    }

    std::memset(resultTable, 0, sizeof(RaceResultEntry) * randomizerMaxCarCount);

    for (int i = 0; i < preservedCount && i < participantCount; ++i) {
        resultTable[i] = preservedFinished[i];
        if (resultTable[i].car != nullptr) {
            resultTable[i].car->finishPosition = i;
        }
    }

    for (int i = 0; i < static_cast<int>(g_knockoutResults.size()); ++i) {
        const int resultIndex = participantCount - 1 - i;
        if (resultIndex < preservedCount || resultIndex < 0) {
            break;
        }

        const KnockoutResultEntry& knockoutEntry = g_knockoutResults[i];
        resultTable[resultIndex].car = knockoutEntry.car;
        resultTable[resultIndex].totalTimeMs = knockoutEntry.knockoutTimeMs;
        resultTable[resultIndex].dnfFlag = 1;

        if (knockoutEntry.car != nullptr) {
            knockoutEntry.car->finishPosition = resultIndex;
            knockoutEntry.car->finishTimeMs = static_cast<int32_t>(knockoutEntry.knockoutTimeMs);
        }
    }
}

bool IsKnockoutRaceSupported() {
    GameModeRuntime& gameMode = GetGameModeRuntime();
    return gameMode.mode == MODE_SINGLE_RACE;
}

int ClampKnockoutLapCountMode(int mode) {
    return mode != 0 ? 1 : 0;
}

int GetKnockoutTargetCarCount() {
    const RandomizerContext& ctx = GetRandomizerContext();
    if (ctx.carState.carsPerRace > 0) {
        return std::clamp(ctx.carState.carsPerRace, randomizerMinCarCount, randomizerMaxCarCount);
    }

    return std::clamp(GetParticipantCount(), randomizerMinCarCount, randomizerMaxCarCount);
}

int CalculateAutomaticKnockoutLapCount() {
    RandomizerContext& ctx = GetRandomizerContext();
    const int carCount = GetKnockoutTargetCarCount();
    const int frequency = std::clamp(ctx.knockoutState.eliminationFrequencyLaps, 1, 10);
    const int eliminationsPerEvent =
        std::clamp(ctx.knockoutState.eliminationsPerEvent, 1, randomizerMaxCarCount - 1);
    const int eliminationEvents = (carCount - 1 + eliminationsPerEvent - 1) / eliminationsPerEvent;

    return (std::max)(eliminationEvents * frequency, 1);
}

void ApplyKnockoutLapCountOption() {
    RandomizerContext& ctx = GetRandomizerContext();
    ctx.knockoutState.lapCountMode = ClampKnockoutLapCountMode(ctx.knockoutState.lapCountMode);
    if (ctx.knockoutState.lapCountMode == 0) {
        return;
    }

    const int automaticLapCount = CalculateAutomaticKnockoutLapCount();
    GameModeRuntime& gameMode = GetGameModeRuntime();
    gameMode.laps = automaticLapCount;

    PlayerRaceInfoRuntime* playerRaceInfo = GetPlayerRaceInfo();
    if (playerRaceInfo != nullptr) {
        playerRaceInfo->laps = automaticLapCount;
    }
}

int GetEventEliminationCount(int activeCarCount) {
    RandomizerContext& ctx = GetRandomizerContext();
    const int configuredCount =
        std::clamp(ctx.knockoutState.eliminationsPerEvent, 1, randomizerMaxCarCount - 1);
    return (std::min)(configuredCount, activeCarCount - 1);
}

void SortWorstFirst(std::vector<CarEntityRuntime*>& cars) {
    std::sort(
        cars.begin(),
        cars.end(),
        [](const CarEntityRuntime* lhs, const CarEntityRuntime* rhs) {
            return lhs->racePositionIndex > rhs->racePositionIndex;
        }
    );
}

bool EliminateCar(CarEntityRuntime* car, std::vector<KnockoutPopupLine>& popupLines) {
    if (!IsCarStillRacing(car)) {
        return false;
    }

    RandomizerContext& ctx = GetRandomizerContext();
    const uint32_t finishTime = GetCurrentRaceClockMs();
    const int finishPosition = GetParticipantCount() - 1 - ctx.knockoutState.eliminatedCount;

    RVGL_RegisterFinishTime(reinterpret_cast<int*>(car), finishTime, 1);
    g_knockoutResults.push_back({ car, finishTime });
    popupLines.push_back({ car, finishPosition, finishTime });

    if (IsKnockedOutGhostModeEnabled()) {
        KnockoutCarPose pose = {};
        const bool hasPose = CaptureCarPose(car, pose);
        RVGL_SetCarBehaviourState(car, kCarStatePassiveGhost);
        DisablePassiveGhostPlayback(car);
        if (hasPose) {
            RestoreCarPose(car, pose);
        }
    }
    else {
        car->carState = kCarStateGhost;
    }

    ++ctx.knockoutState.eliminatedCount;

    Logger::TimestampLogf(
        "[KnockoutMode] Eliminated car runtimeId=%d position=%d finishTime=%d",
        car->nCarArrayIndex,
        car->racePositionIndex,
        finishTime
    );

    return true;
}

void FinishWinnerIfRaceResolved(const std::vector<CarEntityRuntime*>& activeCars) {
    if (activeCars.size() != 1) {
        return;
    }

    CarEntityRuntime* winner = activeCars.front();
    if (!IsCarStillRacing(winner)) {
        return;
    }

    RVGL_RegisterFinishTime(reinterpret_cast<int*>(winner), GetCurrentRaceClockMs(), 0);
    RewriteKnockoutResultTable();
    GetRandomizerContext().knockoutState.playerWon = winner->nCarArrayIndex == 0;
    GetRandomizerContext().knockoutState.raceActive = false;
    Logger::TimestampLogf(
        "[KnockoutMode] Race resolved; winner runtimeId=%d",
        winner->nCarArrayIndex
    );
}

void ApplyEliminationEvent(std::vector<CarEntityRuntime*>& activeCars, int targetLap) {
    const int eliminateCount = GetEventEliminationCount(static_cast<int>(activeCars.size()));
    if (eliminateCount <= 0) {
        return;
    }

    std::vector<CarEntityRuntime*> completedLapCars;
    std::vector<CarEntityRuntime*> notCompletedLapCars;
    completedLapCars.reserve(activeCars.size());
    notCompletedLapCars.reserve(activeCars.size());

    for (CarEntityRuntime* car : activeCars) {
        if (GetCompletedLapCount(car) >= targetLap) {
            completedLapCars.push_back(car);
        }
        else {
            notCompletedLapCars.push_back(car);
        }
    }

    if (static_cast<int>(completedLapCars.size()) <
        static_cast<int>(activeCars.size()) - eliminateCount) {
        return;
    }

    std::vector<CarEntityRuntime*> eliminationPool =
        static_cast<int>(notCompletedLapCars.size()) >= eliminateCount
            ? notCompletedLapCars
            : activeCars;
    SortWorstFirst(eliminationPool);

    std::vector<KnockoutPopupLine> popupLines;
    popupLines.reserve(eliminateCount);

    for (int i = 0; i < eliminateCount && i < static_cast<int>(eliminationPool.size()); ++i) {
        EliminateCar(eliminationPool[i], popupLines);
    }

    if (!popupLines.empty()) {
        RewriteKnockoutResultTable();
        ShowKnockoutPopup(popupLines);
    }

    RandomizerContext& ctx = GetRandomizerContext();
    ctx.knockoutState.nextEliminationLap +=
        std::clamp(ctx.knockoutState.eliminationFrequencyLaps, 1, 10);
}

} // anonymous namespace

void ResetKnockoutRaceState() {
    RandomizerContext& ctx = GetRandomizerContext();
    if (ctx.knockoutState.modeActive) {
        ClearNativeKnockoutRaceState();
    }

    ctx.knockoutState.raceActive = false;
    ctx.knockoutState.nextEliminationLap =
        std::clamp(ctx.knockoutState.eliminationFrequencyLaps, 1, 10);
    ctx.knockoutState.eliminatedCount = 0;
    ctx.knockoutState.lastRaceClockMs = 0;
    ctx.knockoutState.playerWon = false;
    g_knockoutResults.clear();
    g_knockoutPopupLines.clear();
    g_knockoutPopupUntilMs = 0;
}

void StartKnockoutRaceIfSelected() {
    RandomizerContext& ctx = GetRandomizerContext();
    if (!ctx.knockoutState.modeActive || !IsKnockoutRaceSupported()) {
        return;
    }

    ClearNativeKnockoutRaceState();
    PatchNativeSingleRaceLabelForKnockout();

    ctx.knockoutState.raceActive = true;
    ctx.knockoutState.menuSelectionActive = true;
    ctx.knockoutState.eliminationFrequencyLaps =
        std::clamp(ctx.knockoutState.eliminationFrequencyLaps, 1, 10);
    ctx.knockoutState.eliminationsPerEvent =
        std::clamp(ctx.knockoutState.eliminationsPerEvent, 1, randomizerMaxCarCount - 1);
    ctx.knockoutState.knockedOutGhostMode = ctx.knockoutState.knockedOutGhostMode != 0 ? 1 : 0;
    ApplyKnockoutLapCountOption();
    ctx.knockoutState.nextEliminationLap = ctx.knockoutState.eliminationFrequencyLaps;
    ctx.knockoutState.eliminatedCount = 0;
    ctx.knockoutState.lastRaceClockMs = 0;
    ctx.knockoutState.playerWon = false;
    g_knockoutPopupLines.clear();
    g_knockoutPopupUntilMs = 0;

    Logger::TimestampLogf(
        "[KnockoutMode] Started frequency=%d eliminations=%d laps=%d",
        ctx.knockoutState.eliminationFrequencyLaps,
        ctx.knockoutState.eliminationsPerEvent,
        GetGameModeRuntime().laps
    );
}

void FinalizeKnockoutRaceSetup() {
    RandomizerContext& ctx = GetRandomizerContext();
    if (!ctx.knockoutState.modeActive || !IsKnockoutRaceSupported()) {
        return;
    }

    ClearNativeKnockoutRaceState();
    PatchNativeSingleRaceLabelForKnockout();
    g_knockoutResults.clear();

    ctx.knockoutState.raceActive = true;
    ctx.knockoutState.menuSelectionActive = true;
    ctx.knockoutState.nextEliminationLap =
        std::clamp(ctx.knockoutState.eliminationFrequencyLaps, 1, 10);
    ctx.knockoutState.knockedOutGhostMode = ctx.knockoutState.knockedOutGhostMode != 0 ? 1 : 0;
    ApplyKnockoutLapCountOption();
    ctx.knockoutState.eliminatedCount = 0;
    ctx.knockoutState.lastRaceClockMs = static_cast<int>(GetCurrentRaceClockMs());
    ctx.knockoutState.playerWon = false;
    g_knockoutPopupLines.clear();
    g_knockoutPopupUntilMs = 0;
}

void UpdateKnockoutRaceProgress() {
    RandomizerContext& ctx = GetRandomizerContext();
    if (!ctx.knockoutState.raceActive) {
        return;
    }

    GameModeRuntime& gameMode = GetGameModeRuntime();
    if (gameMode.mode != MODE_SINGLE_RACE) {
        ctx.knockoutState.raceActive = false;
        return;
    }

    const uint32_t currentRaceClock = GetCurrentRaceClockMs();
    if (ctx.knockoutState.lastRaceClockMs > 0 &&
        currentRaceClock < static_cast<uint32_t>(ctx.knockoutState.lastRaceClockMs)) {
        Logger::TimestampLogf(
            "[KnockoutMode] Race clock reset detected; resetting knockout restart state"
        );
        ResetThirtyCarModState();
        ResetKnockoutRaceState();
        StartKnockoutRaceIfSelected();
        return;
    }

    std::vector<CarEntityRuntime*> activeCars = GetActiveRaceCars();
    if (activeCars.size() <= 1) {
        FinishWinnerIfRaceResolved(activeCars);
        return;
    }

    const int totalLaps = (std::max)(gameMode.laps, 1);
    while (ctx.knockoutState.nextEliminationLap <= totalLaps && activeCars.size() > 1) {
        const int previousEliminatedCount = ctx.knockoutState.eliminatedCount;
        ApplyEliminationEvent(activeCars, ctx.knockoutState.nextEliminationLap);

        if (ctx.knockoutState.eliminatedCount == previousEliminatedCount) {
            break;
        }

        activeCars = GetActiveRaceCars();
    }

    RewriteKnockoutResultTable();
    FinishWinnerIfRaceResolved(activeCars);
    ctx.knockoutState.lastRaceClockMs = static_cast<int>(currentRaceClock);
}

void DrawKnockoutPopup() {
    if (g_knockoutPopupLines.empty()) {
        return;
    }

    const uint32_t currentRaceClock = GetCurrentRaceClockMs();
    if (g_knockoutPopupUntilMs != 0 && currentRaceClock > g_knockoutPopupUntilMs) {
        g_knockoutPopupLines.clear();
        g_knockoutPopupUntilMs = 0;
        return;
    }

    if (IsFrontendMenuPanelActive()) {
        return;
    }

    const int lineCount = static_cast<int>(g_knockoutPopupLines.size());
    const float textWidth = lineCount > 12 ? 6.2f : 8.0f;
    const float textHeight = lineCount > 12 ? 12.0f : 16.0f;
    const float rowHeight = textHeight;
    const float baseY = 440.0f;
    const float startY = std::clamp(
        baseY - static_cast<float>(lineCount - 1) * rowHeight,
        48.0f,
        baseY
    );
    const float rankColumnWidth = 5.0f * textWidth;
    const float nameGap = textWidth;
    float maxNameWidth = 0.0f;

    for (const KnockoutPopupTextLine& line : g_knockoutPopupLines) {
        const int64_t nameLen = RVGL_UTF8GetVisibleCharCount(MutableText(line.driverName));
        maxNameWidth = (std::max)(maxNameWidth, static_cast<float>(nameLen) * textWidth);
    }

    const float totalWidth = rankColumnWidth + nameGap + maxNameWidth;
    const float x = GetCenteredPanelX(totalWidth);
    const float nameX = x + rankColumnWidth + nameGap;

    RVGL_SetupGLRenderState();

    for (int i = 0; i < lineCount; ++i) {
        const KnockoutPopupTextLine& line = g_knockoutPopupLines[i];
        const int64_t rankLen = RVGL_UTF8GetVisibleCharCount(MutableText(line.rankText));
        const float y = startY + static_cast<float>(i) * rowHeight;

        DrawSizedText(
            x + rankColumnWidth - static_cast<float>(rankLen) * textWidth,
            y,
            textWidth,
            textHeight,
            line.color,
            line.rankText
        );
        DrawSizedText(nameX, y, textWidth, textHeight, line.color, line.driverName, maxNameWidth);
    }
}

bool DrawKnockoutResultsTable() {
    RandomizerContext& ctx = GetRandomizerContext();
    if (!ctx.knockoutState.modeActive || g_knockoutResults.empty()) {
        return false;
    }

    if (!IsNativeResultsTableVisible()) {
        return false;
    }

    RaceResultEntry* resultTable = GetRaceResultTable();
    if (resultTable == nullptr) {
        return false;
    }

    const int participantCount = std::clamp(GetParticipantCount(), 0, randomizerMaxCarCount);
    if (participantCount <= 0) {
        return false;
    }

    bool hasResult = false;
    for (int i = 0; i < participantCount; ++i) {
        if (resultTable[i].car != nullptr && resultTable[i].totalTimeMs != 0) {
            hasResult = true;
            break;
        }
    }

    if (!hasResult) {
        return false;
    }

    const float textWidth = participantCount > 27 ? 6.2f :
        (participantCount > 24 ? 7.0f : (participantCount > 20 ? 7.5f : 8.0f));
    const float textHeight = participantCount > 27 ? 11.0f :
        (participantCount > 24 ? 12.5f : (participantCount > 20 ? 14.0f : 16.0f));
    const float rowHeight = textHeight;
    const float contentPaddingX = 4.0f;
    const float contentPaddingY = 4.0f;
    const float panelContentHeight =
        rowHeight * 2.0f + 1.0f + static_cast<float>(participantCount) * rowHeight + contentPaddingY * 2.0f;
    const float panelHeight = std::clamp(panelContentHeight, 115.0f, 362.0f);
    const float panelY = std::clamp(245.0f - panelHeight * 0.5f, 70.0f, 150.0f);
    const float headerY = panelY + contentPaddingY;
    const float rowStartY = headerY + rowHeight * 2.0f + 1.0f;
    const float panelWidth = 200.0f;
    const float panelX = GetCenteredPanelX(panelWidth);
    const float tableX = panelX + contentPaddingX;
    const float rankX = tableX;
    const float nameX = rankX + 20.0f;
    const float timeRightX = tableX + panelWidth - contentPaddingX - 4.0f;

    RVGL_UIDrawRoundedRect(panelX, panelY, panelWidth, panelHeight, 0, 0, 0xb0181818, 0xff, 1);
    RVGL_FlushDeferredUIBatches();
    RVGL_SetupGLRenderState();

    DrawSizedText(tableX, headerY, textWidth, textHeight, 0xff00ffff, LocaleString(0x135, "Race Results"), 140.0f);

    for (int resultIndex = 0; resultIndex < participantCount; ++resultIndex) {
        const RaceResultEntry& entry = resultTable[resultIndex];
        if (entry.car == nullptr || entry.totalTimeMs == 0) {
            continue;
        }

        const float y = rowStartY + static_cast<float>(resultIndex) * rowHeight;

        char rankText[8] = {};
        std::snprintf(rankText, sizeof(rankText), "%2.2d", resultIndex + 1);
        DrawSizedText(rankX, y, textWidth, textHeight, 0xffffffff, rankText);

        DrawSizedText(nameX, y, textWidth, textHeight, 0xff00ffff, entry.car->driverName, 92.0f);

        char timeText[16] = {};
        FormatRaceTime(entry.totalTimeMs, timeText);
        DrawRightSizedText(timeRightX, y, textWidth, textHeight, 0xffffffff, timeText);
    }

    return true;
}

} // namespace Randomizer
