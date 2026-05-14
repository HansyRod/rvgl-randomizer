#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <random>
#include <unordered_map>
#include <vector>
#include "RVGLStructs.h"
#include "Addresses.h"
#include "RandomizerState.h"

namespace Randomizer {

struct Vec3 {
    float x;
    float y;
    float z;
};

#pragma pack(push, 1)
struct PhysicsBodyRuntime {
    uint8_t _pad_00[20];
    Vec3 position;                  // +0x14
    Vec3 velocity;                  // +0x20
    uint8_t _pad_2C[40];
    float orientationMatrix[9];     // +0x54
};

struct CarTransformRuntime {
    int32_t modelId;                // +0x000
    uint8_t _pad_004[124];
    PhysicsBodyRuntime* physicsBody; // +0x080
    uint8_t _pad_088[3416];
    Vec3 cachedPosition;            // +0xDE0
    uint8_t _pad_DEC[204];
};

struct CarEntityRuntime {
    int32_t nCarArrayIndex;         // +0x0000
    int32_t carState;               // +0x0004
    CarEntityRuntime* pPrev;        // +0x0008
    CarEntityRuntime* pNext;        // +0x0010
    uint8_t _pad_018[48];
    CarTransformRuntime transform;  // +0x0048
    uint8_t _pad_0F00[23232];
    int32_t racePositionIndex;      // +0x69C0, zero-based
};

struct RaceParticipantRuntime {
    int32_t carType;                // +0x00
    int32_t startSlot;              // +0x04
    int32_t modelId;                // +0x08
    int32_t skinId;                 // +0x0C
    int32_t reserved10;             // +0x10
    int32_t isLocal;                // +0x14
    int32_t networkId;              // +0x18
    int32_t hasCheated;             // +0x1C
    char carName[20];               // +0x20
    char skinName[12];              // +0x34
    char playerName[16];            // +0x40
};
#pragma pack(pop)

static_assert(sizeof(void*) == 8, "RVGL runtime layouts here assume a 64-bit process.");
static_assert(offsetof(PhysicsBodyRuntime, position) == 0x14, "PhysicsBodyRuntime::position offset mismatch.");
static_assert(offsetof(PhysicsBodyRuntime, orientationMatrix) == 0x54, "PhysicsBodyRuntime::orientationMatrix offset mismatch.");
static_assert(offsetof(CarTransformRuntime, physicsBody) == 0x80, "CarTransformRuntime::physicsBody offset mismatch.");
static_assert(offsetof(CarTransformRuntime, cachedPosition) == 0xDE0, "CarTransformRuntime::cachedPosition offset mismatch.");
static_assert(sizeof(CarTransformRuntime) == 0xEB8, "CarTransformRuntime size mismatch.");
static_assert(offsetof(CarEntityRuntime, transform) == 0x48, "CarEntityRuntime::transform offset mismatch.");
static_assert(offsetof(CarEntityRuntime, racePositionIndex) == 0x69C0, "CarEntityRuntime::racePositionIndex offset mismatch.");
static_assert(sizeof(RaceParticipantRuntime) == 0x50, "RaceParticipantRuntime size mismatch.");
static_assert(offsetof(RaceParticipantRuntime, modelId) == 0x08, "RaceParticipantRuntime::modelId offset mismatch.");
static_assert(offsetof(RaceParticipantRuntime, playerName) == 0x40, "RaceParticipantRuntime::playerName offset mismatch.");

using FnCreateCarEntity = CarEntityRuntime* (__cdecl *)(
    int initialState,
    int controllerType,
    int carModelId,
    uint64_t skinIdAndFlags,
    float* spawnPosition,
    float* spawnOrientation
);

using FnComputeSpawnOrientation = void (__cdecl *)(
    int startSlot,
    float* outPosition,
    float* outOrientation
);

using FnSetCarTransform = void (__cdecl *)(
    CarTransformRuntime* transform,
    float* position,
    float* orientation
);

using FnAddParticipantAndCount = bool (__cdecl *)(
    int carType,
    int startSlot,
    int modelId,
    int skinId,
    int isLocal,
    int networkId,
    char* playerName
);

constexpr int kMinCarCount = 2;
constexpr int kVanillaMaxCarCount = 16;
constexpr int kTargetCarCount = 30;
constexpr int kGridCols = 5;
constexpr int kGridRows = 6;
constexpr float kColumnSpacing = 150.0f;
constexpr float kRowSpacing = 150.0f;
constexpr int kCpuRaceCarState = 3;
constexpr int kNoInputController = 0;


struct ExtraCarsState {
    bool cacheValid = false;
    bool participantsExpanded = false;
    bool gridApplied = false;
    bool playersMovedToBack = false;
    int originalParticipantCount = 0;
    std::array<int, kTargetCarCount> cachedModels = {};
};

ExtraCarsState g_extraCars;

std::mt19937& Rng() {
    static std::mt19937 rng{ std::random_device{}() };
    return rng;
}

namespace {

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
    return std::clamp(ctx.carState.carsPerRace, kMinCarCount, kTargetCarCount);
}

int GetParticipantCount() {
    return *reinterpret_cast<int*>(AbsFromRva(RVA_RACE_PARTICIPANT_COUNT));
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
    auto AddParticipantAndCount = reinterpret_cast<FnAddParticipantAndCount>(
        AbsFromRva(RVA_ADD_PARTICIPANT_AND_COUNT)
    );

    return AddParticipantAndCount(
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

    auto SetCarTransform = reinterpret_cast<FnSetCarTransform>(
        AbsFromRva(RVA_SET_CAR_TRANSFORM)
    );
    SetCarTransform(&car->transform, spawnPosition, spawnOrientation);
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
    std::unordered_map<int, bool> usedModels;

    for (int carId = 0; carId < carCount; ++carId) {
        int modelId = GetParticipantModelId(carId);
        if (modelId < 0) {
            modelId = GetCarModel(carId);
        }

        if (modelId >= 0) {
            usedModels[modelId] = true;
        }
    }

    int playerModelId = GetParticipantModelId(0);
    if (playerModelId < 0) {
        playerModelId = GetCarModel(0);
    }

    const int targetRating = GetCarModelRating(playerModelId >= 0 ? playerModelId : 0);

    const int targetCarCount = GetTargetRaceCarCount();
    for (int carId = carCount; carId < targetCarCount; ++carId) {
        g_extraCars.cachedModels[carId] = PickRandomModel(targetRating, usedModels);
    }

    g_extraCars.cacheValid = true;
}

int SpawnCar(int modelId, int skinId, const Vec3& pos) {
    auto CreateCarEntity = reinterpret_cast<FnCreateCarEntity>(
        AbsFromRva(RVA_CREATE_CAR_ENTITY)
    );

    auto ComputeSpawnOrientation = reinterpret_cast<FnComputeSpawnOrientation>(
        AbsFromRva(RVA_COMPUTE_SPAWN_ORIENT)
    );

    float spawnPosition[3] = { pos.x, pos.y, pos.z };

    float ignoredNativePos[3];
    float spawnOrientation[12];

    // Use slot 0 only for orientation. Do not pass gridIndex >= 16 here:
    // the normal race path indexes a 16-entry start-position table.
    ComputeSpawnOrientation(0, ignoredNativePos, spawnOrientation);

    CarEntityRuntime* car = CreateCarEntity(
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
    if (!IsSupportedMode() || g_extraCars.participantsExpanded) {
        return;
    }

    const int participantCount = GetParticipantCount();
    if (participantCount <= 0) {
        return;
    }

    const int targetCarCount = GetTargetRaceCarCount();
    if (participantCount >= targetCarCount) {
        g_extraCars.originalParticipantCount = participantCount;
        g_extraCars.participantsExpanded = true;
        return;
    }

    if (!g_extraCars.cacheValid) {
        CacheRandomModels(participantCount);
    }

    g_extraCars.originalParticipantCount = participantCount;

    for (int participantIndex = participantCount; participantIndex < targetCarCount; ++participantIndex) {
        const int modelId = g_extraCars.cachedModels[participantIndex];
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

    g_extraCars.participantsExpanded = true;
}

void ApplyThirtyCarGrid() {
    if (!IsSupportedMode()) {
        return;
    }

    const int targetCarCount = GetTargetRaceCarCount();
    const int carCount = GetParticipantCount();
    if (carCount <= 0 || carCount > targetCarCount) {
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

    if (!g_extraCars.cacheValid) {
        CacheRandomModels(carCount);
    }

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
            continue;
        }

        const int modelId = g_extraCars.cachedModels[gridIndex];
        const int newCarId = SpawnCar(modelId, 0, pos);
        (void)newCarId;
    }

    g_extraCars.gridApplied = true;
}

void MovePlayersToBackAfterRacePositions() {
    if (!IsSupportedMode() || !g_extraCars.gridApplied || g_extraCars.playersMovedToBack) {
        return;
    }

    std::array<int, kTargetCarCount + 1> rankToCar;
    rankToCar.fill(-1);

    const int targetCarCount = GetTargetRaceCarCount();
    for (int carId = 0; carId < targetCarCount; ++carId) {
        const int rank = GetCarRankingPosition(carId);
        if (rank >= 1 && rank <= targetCarCount) {
            rankToCar[rank] = carId;
        }
    }

    const int playerCars[1] = { 0 };
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
        g_extraCars.playersMovedToBack = true;
    }
}

void ResetThirtyCarModState() {
    g_extraCars.cacheValid = false;
    g_extraCars.participantsExpanded = false;
    g_extraCars.gridApplied = false;
    g_extraCars.playersMovedToBack = false;
    g_extraCars.originalParticipantCount = 0;
    g_extraCars.cachedModels.fill(0);
}

void SyncCarCountToVanillaSettings() {

    RandomizerContext& ctx = GetRandomizerContext();
    int carsPerRace = std::clamp(ctx.carState.carsPerRace, kMinCarCount, kTargetCarCount);
    ctx.carState.carsPerRace = carsPerRace;

    int& nCars = *reinterpret_cast<int*>(AbsFromRva(RVA_NCARS));
    int& nCarsSetting = *reinterpret_cast<int*>(AbsFromRva(RVA_SETTINGS_NCARS));
    const int vanillaCarCount = carsPerRace < kVanillaMaxCarCount
        ? carsPerRace
        : kVanillaMaxCarCount;

    nCars = vanillaCarCount;
    nCarsSetting = vanillaCarCount;
}

}
