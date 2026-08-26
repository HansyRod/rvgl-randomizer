#include "GridShuffle.h"

#include <algorithm>
#include <random>
#include <vector>

#include "30CarMod.h"
#include "CupHooks.h"
#include "RandomizerState.h"
#include "RVGLMemory.h"
#include "RVGLStructs.h"
#include "ThirtyCarCupMod.h"

namespace Randomizer {

namespace {

constexpr int kNativeCupCars = vanillaMaxCarCount;
constexpr int kMaxGridCars = randomizerMaxCarCount;

struct GridSlot {
    Vec3 position;
    Vec3 forwardDirection;
};

std::mt19937& GridShuffleRng() {
    static std::mt19937 rng{ std::random_device{}() };
    return rng;
}

bool ReadGridSlot(int carId, GridSlot& slot) {
    const CarEntityRuntime* car = GetLiveCarById(carId);
    if (car == nullptr || car->transform.physicsBody == nullptr) {
        return false;
    }

    slot.position = car->transform.physicsBody->position;
    slot.forwardDirection = {
        car->transform.physicsBody->orientationMatrix[6],
        0.0f,
        car->transform.physicsBody->orientationMatrix[8]
    };
    return true;
}

void ShuffleGridCars(const std::vector<int>& carIds) {
    if (carIds.empty()) {
        return;
    }

    std::vector<GridSlot> shuffledSlots;
    shuffledSlots.reserve(carIds.size());
    for (const int carId : carIds) {
        GridSlot slot;
        if (!ReadGridSlot(carId, slot)) {
            return;
        }
        shuffledSlots.push_back(slot);
    }

    std::shuffle(shuffledSlots.begin(), shuffledSlots.end(), GridShuffleRng());

    for (size_t gridIndex = 0; gridIndex < carIds.size(); ++gridIndex) {
        SetCarPosAndForwardDirection(
            carIds[gridIndex],
            shuffledSlots[gridIndex].position,
            shuffledSlots[gridIndex].forwardDirection
        );
    }
}

} // anonymous namespace

void ApplyStartingGridShuffle() {
    std::vector<int> gridCarIds;
    const ThirtyCarRuntimeState& state = GetRandomizerContext().thirtyCarState;

    // Expanded single races/knockouts may spawn cars with runtime IDs that do
    // not match their participant indexes; expanded cups retain index-based IDs.
    if (!IsThirtyCarCupActive() && state.gridApplied) {
        gridCarIds.reserve(randomizerMaxCarCount);
        for (const int runtimeCarId : state.runtimeCarIds) {
            if (runtimeCarId >= 0) {
                gridCarIds.push_back(runtimeCarId);
            }
        }
    }
    else {
        const int participantCount = GetParticipantCount();
        gridCarIds.reserve(participantCount);
        for (int carId = 0; carId < participantCount; ++carId) {
            gridCarIds.push_back(carId);
        }
    }

    ShuffleGridCars(gridCarIds);
}

} // namespace Randomizer
