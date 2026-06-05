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
#include <vector>

namespace Randomizer {

namespace {

constexpr int kCarStateInactive = 0;
constexpr int kCarStateEliminated = 9;
constexpr int kCompletedLapOffset = 0x0F40;
constexpr int kMaxCarListTraversal = 128;
constexpr uint32_t kDefaultPopupPrefixColor = 0xff00ffff;
constexpr uint32_t kDefaultPopupTimeColor = 0xffffff;

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

std::vector<KnockoutResultEntry> g_knockoutResults;

int ReadCarInt(CarEntityRuntime* car, int offset) {
    return *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(car) + offset);
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

void ShowKnockoutPopup(CarEntityRuntime* car, int zeroBasedPosition, uint32_t knockoutTimeMs) {
    char prefix[64] = {};
    char timeText[16] = {};
    FormatRaceTime(knockoutTimeMs, timeText);

    std::snprintf(
        prefix,
        sizeof(prefix),
        "%d%s: %s: ",
        zeroBasedPosition + 1,
        GetFinishSuffix(zeroBasedPosition),
        car != nullptr ? car->driverName : ""
    );

    RVGL_SetPostRacePopup(
        prefix,
        timeText,
        GetCarPopupColor(car),
        kDefaultPopupTimeColor,
        1,
        2.0f
    );
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
    return gameMode.mode == MODE_SINGLE_RACE && GetParticipantCount() > 1;
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

void EliminateCar(CarEntityRuntime* car) {
    if (!IsCarStillRacing(car)) {
        return;
    }

    RandomizerContext& ctx = GetRandomizerContext();
    const uint32_t finishTime = GetCurrentRaceClockMs();
    const int finishPosition = GetParticipantCount() - 1 - ctx.knockoutState.eliminatedCount;

    RVGL_RegisterFinishTime(reinterpret_cast<int*>(car), finishTime, 1);
    g_knockoutResults.push_back({ car, finishTime });
    RewriteKnockoutResultTable();
    ShowKnockoutPopup(car, finishPosition, finishTime);

    car->carState = kCarStateEliminated;
    ++ctx.knockoutState.eliminatedCount;

    Logger::TimestampLogf(
        "[KnockoutMode] Eliminated car runtimeId=%d position=%d finishTime=%d",
        car->nCarArrayIndex,
        car->racePositionIndex,
        finishTime
    );
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

    for (int i = 0; i < eliminateCount && i < static_cast<int>(eliminationPool.size()); ++i) {
        EliminateCar(eliminationPool[i]);
    }

    RandomizerContext& ctx = GetRandomizerContext();
    ctx.knockoutState.nextEliminationLap +=
        std::clamp(ctx.knockoutState.eliminationFrequencyLaps, 1, 10);
}

} // anonymous namespace

void ResetKnockoutRaceState() {
    RandomizerContext& ctx = GetRandomizerContext();
    ctx.knockoutState.raceActive = false;
    ctx.knockoutState.nextEliminationLap =
        std::clamp(ctx.knockoutState.eliminationFrequencyLaps, 1, 10);
    ctx.knockoutState.eliminatedCount = 0;
    g_knockoutResults.clear();
}

void StartKnockoutRaceIfSelected() {
    RandomizerContext& ctx = GetRandomizerContext();
    if (!ctx.knockoutState.menuSelectionActive || !IsKnockoutRaceSupported()) {
        return;
    }

    ctx.knockoutState.raceActive = true;
    ctx.knockoutState.eliminationFrequencyLaps =
        std::clamp(ctx.knockoutState.eliminationFrequencyLaps, 1, 10);
    ctx.knockoutState.eliminationsPerEvent =
        std::clamp(ctx.knockoutState.eliminationsPerEvent, 1, randomizerMaxCarCount - 1);
    ctx.knockoutState.nextEliminationLap = ctx.knockoutState.eliminationFrequencyLaps;
    ctx.knockoutState.eliminatedCount = 0;

    Logger::TimestampLogf(
        "[KnockoutMode] Started frequency=%d eliminations=%d",
        ctx.knockoutState.eliminationFrequencyLaps,
        ctx.knockoutState.eliminationsPerEvent
    );
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
}

} // namespace Randomizer
