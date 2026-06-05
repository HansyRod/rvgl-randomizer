#include "CupOpponentGrid.h"
#include "30CarMod.h"
#include "CupHooks.h"
#include "Logger.h"
#include "RandomizerState.h"
#include "RVGLMemory.h"
#include "ThirtyCarCupMod.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace Randomizer {

FnCup_GenerateOpponentGrid Orig_Cup_GenerateOpponentGrid = nullptr;

namespace {

constexpr int kMaxCupCars = randomizerMaxCarCount;
constexpr int kNativeCupCars = vanillaMaxCarCount;

const CarInfo* GetCarInfoByModelId(int modelId) {
    CarInfo* cars = GetCarInfoTable();
    if (cars == nullptr || modelId < 0 || modelId >= GetTotalCarModelCount()) {
        return nullptr;
    }

    return &cars[modelId];
}

int GetSelectedCupIndexFromSettings() {
    RaceSettingsRuntime* settings = GetRaceSettings();
    return settings != nullptr ? settings->selectedCupIndex : -1;
}

int GetSelectedPlayerModelFromSettings() {
    RaceSettingsRuntime* settings = GetRaceSettings();
    return settings != nullptr ? settings->playerModelId : 0;
}

int GetSelectedPlayerSkinFromSettings() {
    RaceSettingsRuntime* settings = GetRaceSettings();
    return settings != nullptr ? settings->playerSkinId : 0;
}

CupProfile* ResolveActiveCupFromSelection(int selectedCupIndex) {
    return GetCupProfileByCupID(selectedCupIndex);
}

const RandomizedCup* FindCupConfig(int selectedCupIndex) {
    return GetCupConfigByCupID(selectedCupIndex);
}

bool HasConfiguredOpponents(const RandomizedCup* cupConfig) {
    return cupConfig != nullptr &&
           cupConfig->opponents.has_value() &&
           !cupConfig->opponents->empty();
}

bool ShouldGenerateOpponentGrid(CupProfile* cup, const RandomizedCup* cupConfig) {
    return cup != nullptr &&
           cup->numCars > 0 &&
           cup->numCars <= kMaxCupCars &&
           (IsExtendedCupOpponentGrid(cup) || HasConfiguredOpponents(cupConfig));
}

int FindCarModelIdByFolderName(const std::string& folderName) {
    if (folderName.empty()) {
        return -1;
    }

    CarInfo* cars = GetCarInfoTable();
    if (cars == nullptr) {
        return -1;
    }

    const int totalModels = GetTotalCarModelCount();
    for (int modelId = 0; modelId < totalModels; ++modelId) {
        const CarInfo& car = cars[modelId];
        if (!car.isInvalid && _stricmp(car.internalName, folderName.c_str()) == 0) {
            return modelId;
        }
    }

    return -1;
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

std::vector<int> BuildRequestedRatings(CupProfile* cup, int count, int playerModelId) {
    std::vector<int> requestedRatings;
    if (cup == nullptr || count <= 1) {
        return requestedRatings;
    }

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

    if (static_cast<int>(requestedRatings.size()) > count - 1) {
        requestedRatings.resize(count - 1);
    }

    return requestedRatings;
}

std::array<int, 6> CountRatingSlots(const std::vector<int>& requestedRatings) {
    std::array<int, 6> ratingSlots = {};
    for (int rating : requestedRatings) {
        if (rating >= 0 && rating < static_cast<int>(ratingSlots.size())) {
            ++ratingSlots[rating];
        }
    }

    return ratingSlots;
}

void AssignCupParticipant(
    ExtendedCupResultsState& results,
    int participantIndex,
    int modelId,
    int skinId
) {
    results.participants[participantIndex].participantIndex = participantIndex;
    results.participants[participantIndex].modelId = modelId;
    results.participants[participantIndex].skinId = skinId;
}

int AddConfiguredOpponents(
    const RandomizedCup* cupConfig,
    ExtendedCupResultsState& results,
    int nextParticipantIndex,
    int count,
    int playerModelId,
    std::array<int, 6>& remainingRatingSlots,
    std::unordered_map<int, bool>& usedModels
) {
    if (!HasConfiguredOpponents(cupConfig)) {
        return nextParticipantIndex;
    }

    for (const std::string& folderName : cupConfig->opponents.value()) {
        if (nextParticipantIndex >= count) {
            break;
        }

        const int modelId = FindCarModelIdByFolderName(folderName);
        if (modelId < 0) {
            Logger::TimestampLogf(
                "[CupOpponentGrid] Warning: Configured opponent car folder '%s' was not found.",
                folderName.c_str()
            );
            continue;
        }

        if (modelId == playerModelId || usedModels.find(modelId) != usedModels.end()) {
            continue;
        }

        const int rating = GetCarModelRating(modelId);
        if (rating < 0 ||
            rating >= static_cast<int>(remainingRatingSlots.size()) ||
            remainingRatingSlots[rating] <= 0) {
            continue;
        }

        AssignCupParticipant(
            results,
            nextParticipantIndex,
            modelId,
            PickCpuSkinForCup(modelId)
        );
        usedModels[modelId] = true;
        --remainingRatingSlots[rating];
        ++nextParticipantIndex;
    }

    return nextParticipantIndex;
}

void FillRemainingCupOpponents(
    ExtendedCupResultsState& results,
    int nextParticipantIndex,
    int count,
    int playerModelId,
    const std::array<int, 6>& remainingRatingSlots,
    std::unordered_map<int, bool>& usedModels
) {
    std::vector<int> remainingRatings;
    for (int rating = 0; rating < static_cast<int>(remainingRatingSlots.size()); ++rating) {
        for (int i = 0; i < remainingRatingSlots[rating]; ++i) {
            remainingRatings.push_back(rating);
        }
    }

    for (int i = nextParticipantIndex; i < count; ++i) {
        const int ratingIndex = i - nextParticipantIndex;
        const int rating = ratingIndex < static_cast<int>(remainingRatings.size())
            ? remainingRatings[ratingIndex]
            : GetCarModelRating(playerModelId);
        const int modelId = PickCupOpponentModel(rating, playerModelId, usedModels);
        AssignCupParticipant(results, i, modelId, PickCpuSkinForCup(modelId));
    }
}

void ResetCupRuntimeState() {
    GetCurrentCupIndex() = 0;
    GetCurrentCupStageIndex() = 0;
    GetCupTriesLeft() = 0;
    GetCupStageDirection() = 0;
    GetCupPostRaceState() = 0;
    GetCupResultRuntime() = {};

    CupParticipantEntry* nativeParticipants = GetNativeCupParticipants();
    CupParticipantEntry* nativeStandings = GetNativeCupStandings();
    for (int i = 0; i < kNativeCupCars; ++i) {
        nativeParticipants[i] = {};
        nativeStandings[i] = {};
    }
}

ExtendedCupResultsState GenerateOpponentRoster(
    CupProfile* cup,
    const RandomizedCup* cupConfig,
    int count,
    int playerModelId,
    int playerSkinId
) {
    ExtendedCupResultsState results;
    if (cupConfig != nullptr && !cupConfig->pointsTable.empty()) {
        results.pointsTable = cupConfig->pointsTable;
    }
    else {
        results.pointsTable.assign(cup->pointsTable, cup->pointsTable + kNativeCupCars);
    }

    std::unordered_map<int, bool> usedModels;
    AssignCupParticipant(results, 0, playerModelId, playerSkinId);
    usedModels[playerModelId] = true;

    const std::vector<int> requestedRatings = BuildRequestedRatings(cup, count, playerModelId);
    std::array<int, 6> remainingRatingSlots = CountRatingSlots(requestedRatings);
    const int nextParticipantIndex = AddConfiguredOpponents(
        cupConfig,
        results,
        1,
        count,
        playerModelId,
        remainingRatingSlots,
        usedModels
    );
    FillRemainingCupOpponents(
        results,
        nextParticipantIndex,
        count,
        playerModelId,
        remainingRatingSlots,
        usedModels
    );

    SortExtendedCupStandings(cup, results);
    MirrorExtendedCupTables(cup, results);
    return results;
}

} // anonymous namespace

bool IsExtendedCupOpponentGrid(CupProfile* cup) {
    return cup != nullptr && cup->numCars > kNativeCupCars && cup->numCars <= kMaxCupCars;
}

void Hook_Cup_GenerateOpponentGrid() {
    const int selectedCupIndex = GetSelectedCupIndexFromSettings();
    CupProfile* cup = ResolveActiveCupFromSelection(selectedCupIndex);
    const RandomizedCup* cupConfig = FindCupConfig(selectedCupIndex);
    if (!ShouldGenerateOpponentGrid(cup, cupConfig)) {
        ResetThirtyCarCupState();
        Orig_Cup_GenerateOpponentGrid();
        return;
    }

    const bool extendedCup = IsExtendedCupOpponentGrid(cup);
    ResetCupRuntimeState();

    GetActiveCupRef() = cup;
    GetCurrentCupIndex() = selectedCupIndex;
    GetCupTriesLeft() = cup->numTries;

    const int count = std::clamp(cup->numCars, 1, extendedCup ? kMaxCupCars : kNativeCupCars);
    ExtendedCupResultsState results = GenerateOpponentRoster(
        cup,
        cupConfig,
        count,
        GetSelectedPlayerModelFromSettings(),
        GetSelectedPlayerSkinFromSettings()
    );

    if (extendedCup) {
        StartThirtyCarCupState(selectedCupIndex, cup, cupConfig, results);
    }
    else {
        ResetThirtyCarCupState();
    }

    Logger::TimestampLogf(
        "[CupOpponentGrid] Generated %d-car cup roster for '%s'",
        count,
        cup->displayName
    );
}

} // namespace Randomizer
