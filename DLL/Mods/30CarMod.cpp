#include <algorithm>
#include <array>
#include <cstdio>
#include <random>
#include <unordered_map>
#include <vector>
#include "RVGLFunctions.h"
#include "RVGLStructs.h"
#include "Addresses.h"
#include "RaceInitHooks.h"
#include "RandomizerState.h"

namespace Randomizer {

constexpr int kGridCols = 5;
constexpr int kGridRows = 6;
constexpr float kColumnSpacing = 150.0f;
constexpr float kRowSpacing = 150.0f;
constexpr int kCpuRaceCarState = 3;
constexpr int kNoInputController = 0;

std::mt19937& Rng() {
    static std::mt19937 rng{ std::random_device{}() };
    return rng;
}

int GetParticipantCount() {
    return *reinterpret_cast<int*>(AbsFromRva(RVA_RACE_PARTICIPANT_COUNT));
}

namespace {

ThirtyCarRuntimeState& GetThirtyCarState() {
    return GetRandomizerContext().thirtyCarState;
}

CarInfo* GetCarInfoTable() {
    return *reinterpret_cast<CarInfo**>(AbsFromRva(RVA_CAR_TABLE));
}

bool IsValidModelId(int modelId) {
    const int totalModels = *reinterpret_cast<int*>(AbsFromRva(RVA_CAR_COUNT));
    return modelId >= 0 && modelId < totalModels && GetCarInfoTable() != nullptr;
}

const CarInfo* GetCarInfoByModelId(int modelId) {
    if (!IsValidModelId(modelId)) {
        return nullptr;
    }

    return &GetCarInfoTable()[modelId];
}

CarEntityRuntime* GetLiveCarById(int runtimeCarId) {
    CarEntityRuntime* car = *reinterpret_cast<CarEntityRuntime**>(
        AbsFromRva(RVA_CAR_LIST_HEAD)
    );

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

RaceParticipantRuntime* GetParticipantRecords() {
    return reinterpret_cast<RaceParticipantRuntime*>(
        AbsFromRva(RVA_RACE_PARTICIPANT_RECORDS)
    );
}

int GetTargetRaceCarCount() {
    const RandomizerContext& ctx = GetRandomizerContext();
    return std::clamp(ctx.carState.carsPerRace, randomizerMinCarCount, randomizerMaxCarCount);
}

int GetParticipantModelId(int participantIndex) {
    const int participantCount = GetParticipantCount();
    if (participantIndex < 0 || participantIndex >= participantCount) {
        return -1;
    }

    return GetParticipantRecords()[participantIndex].modelId;
}

bool AddRaceParticipant(
    int carType,
    int startSlot,
    int modelId,
    int skinId,
    int isLocal,
    int networkId,
    char* playerName
) {
    return Orig_AddParticipantAndCount(
        carType,
        startSlot,
        modelId,
        skinId,
        isLocal,
        networkId,
        playerName
    );
}

void BuildParticipantNameFromModel(int modelId, char (&outName)[16]) {
    const CarInfo* carInfo = GetCarInfoByModelId(modelId);
    if (carInfo == nullptr || carInfo->displayName[0] == '\0') {
        std::snprintf(outName, sizeof(outName), "Car %02d", modelId);
        return;
    }

    std::snprintf(outName, sizeof(outName), "%.*s", 15, carInfo->displayName);
}

} // anonymous namespace

int GetTotalCarModelCount() {
    return *reinterpret_cast<int*>(AbsFromRva(RVA_CAR_COUNT));
}

Vec3 GetCarPos(int carId) {
    const CarEntityRuntime* car = GetLiveCarById(carId);
    if (car == nullptr || car->transform.physicsBody == nullptr) {
        return { 0.0f, 0.0f, 0.0f };
    }

    return car->transform.physicsBody->position;
}

void SetCarPos(int carId, const Vec3& pos) {
    CarEntityRuntime* car = GetLiveCarById(carId);
    if (car == nullptr) {
        return;
    }

    if (car->transform.physicsBody == nullptr) {
        car->transform.cachedPosition = pos;
        return;
    }

    float spawnPosition[3] = { pos.x, pos.y, pos.z };
    float spawnOrientation[12] = {};
    for (int i = 0; i < 9; ++i) {
        spawnOrientation[i] = car->transform.physicsBody->orientationMatrix[i];
    }

    RVGL_SetCarTransform(&car->transform, spawnPosition, spawnOrientation);
}

int GetCarModel(int carId) {
    const CarEntityRuntime* car = GetLiveCarById(carId);
    if (car == nullptr) {
        return -1;
    }

    return car->transform.modelId;
}

int GetCarModelRating(int modelId) {
    const CarInfo* carInfo = GetCarInfoByModelId(modelId);
    if (carInfo == nullptr) {
        return ROOKIE;
    }

    return static_cast<int>(carInfo->rating);
}

bool IsCarModelCpuSelectable(int modelId) {
    const CarInfo* carInfo = GetCarInfoByModelId(modelId);
    return carInfo != nullptr && carInfo->selectableByCPU && !carInfo->isInvalid;
}

int GetCarRankingPosition(int carId) {
    const CarEntityRuntime* car = GetLiveCarById(carId);
    if (car == nullptr || car->racePositionIndex < 0) {
        return -1;
    }

    return car->racePositionIndex + 1;
}

bool IsSupportedMode() {
    GameMode* gameMode = reinterpret_cast<GameMode*>(AbsFromRva(RVA_GAME_MODE));
    return *gameMode == MODE_SINGLE_RACE;
}

int PickRandomFromPool(const std::vector<int>& pool) {
    std::uniform_int_distribution<int> dist(0, static_cast<int>(pool.size()) - 1);
    return pool[dist(Rng())];
}

std::vector<int> BuildModelPool(
    int rating,
    bool allowDuplicates,
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

        if (!allowDuplicates && usedModels.find(modelId) != usedModels.end()) {
            continue;
        }

        pool.push_back(modelId);
    }

    return pool;
}

int PickRandomModel(int targetRating, std::unordered_map<int, bool>& usedModels) {
    std::vector<int> pool = BuildModelPool(targetRating, false, usedModels);
    if (!pool.empty()) {
        const int modelId = PickRandomFromPool(pool);
        usedModels[modelId] = true;
        return modelId;
    }

    for (int rating = targetRating - 1; rating >= 0; --rating) {
        pool = BuildModelPool(rating, false, usedModels);
        if (!pool.empty()) {
            const int modelId = PickRandomFromPool(pool);
            usedModels[modelId] = true;
            return modelId;
        }
    }

    pool = BuildModelPool(targetRating, true, usedModels);
    if (!pool.empty()) {
        return PickRandomFromPool(pool);
    }

    const int playerModelId = GetCarModel(0);
    return playerModelId >= 0 ? playerModelId : 0;
}

void CacheRandomModels(int carCount) {
    ThirtyCarRuntimeState& state = GetThirtyCarState();
    std::unordered_map<int, bool> usedModels;

    state.generatedModelIds.fill(-1);

    const int targetCarCount = GetTargetRaceCarCount();
    const int cachedCarCount = carCount < targetCarCount ? carCount : targetCarCount;
    for (int carId = 0; carId < cachedCarCount; ++carId) {
        int modelId = GetParticipantModelId(carId);
        if (modelId < 0) {
            modelId = GetCarModel(carId);
        }

        state.generatedModelIds[carId] = modelId;
        if (modelId >= 0) {
            usedModels[modelId] = true;
        }
    }

    int playerModelId = cachedCarCount > 0 ? state.generatedModelIds[0] : -1;
    if (playerModelId < 0) {
        playerModelId = GetCarModel(0);
    }

    const int targetRating = GetCarModelRating(playerModelId >= 0 ? playerModelId : 0);
    for (int carId = cachedCarCount; carId < targetCarCount; ++carId) {
        state.generatedModelIds[carId] = PickRandomModel(targetRating, usedModels);
    }

    state.cacheValid = true;
}

int SpawnCar(int modelId, int skinId, const Vec3& pos) {
    float spawnPosition[3] = { pos.x, pos.y, pos.z };

    float ignoredNativePos[3];
    float spawnOrientation[12];

    // Use slot 0 only for orientation. Do not pass gridIndex >= 16 here:
    // the normal race path indexes a 16-entry start-position table.
    RVGL_ComputeSpawnOrientation(0, ignoredNativePos, spawnOrientation);

    CarEntityRuntime* car = RVGL_CreateCarEntity(
        kCpuRaceCarState,
        kNoInputController,
        modelId,
        static_cast<uint32_t>(skinId),
        spawnPosition,
        spawnOrientation
    );

    if (!car) {
        return -1;
    }

    return car->nCarArrayIndex;
}

void ExpandRaceParticipantsToThirty() {
    ThirtyCarRuntimeState& state = GetThirtyCarState();
    if (!IsSupportedMode() || state.participantsExpanded) {
        return;
    }

    const int participantCount = GetParticipantCount();
    if (participantCount <= 0) {
        return;
    }

    const int targetCarCount = GetTargetRaceCarCount();
    if (!state.cacheValid) {
        CacheRandomModels(participantCount);
    }

    if (participantCount >= targetCarCount) {
        state.originalParticipantCount = participantCount;
        state.participantsExpanded = true;
        return;
    }

    state.originalParticipantCount = participantCount;

    for (int participantIndex = participantCount; participantIndex < targetCarCount; ++participantIndex) {
        const int modelId = state.generatedModelIds[participantIndex];
        char playerName[16] = {};
        BuildParticipantNameFromModel(modelId, playerName);

        AddRaceParticipant(
            kCpuRaceCarState,
            0,
            modelId,
            0,
            0,
            0,
            playerName
        );
    }

    state.participantsExpanded = true;
}

void ApplyThirtyCarGrid() {
    ThirtyCarRuntimeState& state = GetThirtyCarState();
    if (!IsSupportedMode()) {
        return;
    }

    const int targetCarCount = GetTargetRaceCarCount();
    const int carCount = GetParticipantCount();
    if (carCount <= 16 || carCount > targetCarCount) {
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

    if (!state.cacheValid) {
        CacheRandomModels(carCount);
    }

    state.runtimeCarIds.fill(-1);

    const float gridCenterCol = static_cast<float>(kGridCols - 1) / 2.0f;
    const float gridCenterRow = static_cast<float>(kGridRows - 1) / 2.0f;

    for (int gridIndex = 0; gridIndex < targetCarCount; ++gridIndex) {
        const int row = gridIndex / kGridCols;
        const int col = gridIndex % kGridCols;

        Vec3 pos;
        pos.x = center.x + (static_cast<float>(col) - gridCenterCol) * kColumnSpacing;
        pos.y = center.y;
        pos.z = center.z + (static_cast<float>(row) - gridCenterRow) * kRowSpacing;

        if (gridIndex < carCount) {
            SetCarPos(gridIndex, pos);
            state.runtimeCarIds[gridIndex] = gridIndex;
            continue;
        }

        const int modelId = state.generatedModelIds[gridIndex];
        state.runtimeCarIds[gridIndex] = SpawnCar(modelId, 0, pos);
    }

    state.gridApplied = true;
}

bool MoveRuntimeCarsToBackAfterRacePositions(
    const std::array<int, randomizerMaxCarCount>& runtimeCarIds,
    int targetCarCount
) {
    if (targetCarCount <= 0 || targetCarCount > randomizerMaxCarCount) {
        return false;
    }

    std::array<int, randomizerMaxCarCount + 1> rankToCar;
    rankToCar.fill(-1);

    for (int slot = 0; slot < targetCarCount; ++slot) {
        const int runtimeCarId = runtimeCarIds[slot];
        if (runtimeCarId < 0) {
            continue;
        }

        const int rank = GetCarRankingPosition(runtimeCarId);
        if (rank >= 1 && rank <= targetCarCount) {
            rankToCar[rank] = runtimeCarId;
        }
    }

    const int playerCars[1] = {
        runtimeCarIds[0] >= 0 ? runtimeCarIds[0] : 0
    };
    bool swappedAnyCar = false;

    for (int i = 0; i < 1; ++i) {
        const int playerCarId = playerCars[i];
        const int lastPlaceCarId = rankToCar[targetCarCount - i];

        if (lastPlaceCarId < 0) {
            continue;
        }

        const Vec3 playerPos = GetCarPos(playerCarId);
        const Vec3 lastPlacePos = GetCarPos(lastPlaceCarId);

        SetCarPos(playerCarId, lastPlacePos);
        SetCarPos(lastPlaceCarId, playerPos);
        swappedAnyCar = true;
    }

    if (swappedAnyCar) {
        return true;
    }

    return false;
}

void MovePlayersToBackAfterRacePositions() {
    ThirtyCarRuntimeState& state = GetThirtyCarState();
    if (!IsSupportedMode() || !state.gridApplied || state.playersMovedToBack) {
        return;
    }

    if (MoveRuntimeCarsToBackAfterRacePositions(state.runtimeCarIds, GetTargetRaceCarCount())) {
        state.playersMovedToBack = true;
    }
}

void ResetThirtyCarModState() {
    ThirtyCarRuntimeState& state = GetThirtyCarState();
    state.cacheValid = false;
    state.participantsExpanded = false;
    state.gridApplied = false;
    state.playersMovedToBack = false;
    state.originalParticipantCount = 0;
    state.generatedModelIds.fill(-1);
    state.runtimeCarIds.fill(-1);
}

} // namespace Randomizer
