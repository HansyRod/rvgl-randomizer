#include "GridShuffle.h"

#include <algorithm>
#include <random>
#include <vector>

#include "30CarMod.h"
#include "CupOpponentGrid.h"
#include "RandomizerState.h"
#include "RVGLMemory.h"
#include "RVGLStructs.h"
#include "ThirtyCarCupMod.h"

namespace Randomizer {

namespace {

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

bool ShuffleGridCars(
    const std::vector<int>& carIds,
    int playerCarId
) {
    if (carIds.empty()) {
        return false;
    }

    if (carIds.size() > randomizerMaxCarCount) {
        return false;
    }

    if (std::find(carIds.begin(), carIds.end(), playerCarId) == carIds.end()) {
        return false;
    }

    std::vector<GridSlot> gridSlots;
    gridSlots.reserve(carIds.size());
    for (const int carId : carIds) {
        GridSlot slot;
        if (!ReadGridSlot(carId, slot)) {
            return false;
        }
        gridSlots.push_back(slot);
    }

    // The final slot is reserved for the player. Shuffle only the other
    // positions so the player's placement cannot be changed by this pass.
    std::vector<GridSlot> opponentSlots(gridSlots.begin(), gridSlots.end() - 1);
    std::shuffle(opponentSlots.begin(), opponentSlots.end(), GridShuffleRng());

    size_t opponentAssignment = 0;
    for (const int carId : carIds) {
        if (carId == playerCarId) {
            continue;
        }

        const GridSlot& slot = opponentSlots[opponentAssignment++];
        SetCarPosAndForwardDirection(carId, slot.position, slot.forwardDirection);
    }

    const GridSlot& playerSlot = gridSlots.back();
    SetCarPosAndForwardDirection(
        playerCarId,
        playerSlot.position,
        playerSlot.forwardDirection
    );

    return true;
}

} // anonymous namespace

void ApplyStartingGridShuffle() {
    std::vector<int> gridCarIds;
    ThirtyCarRuntimeState& state = GetRandomizerContext().thirtyCarState;
    const bool expandedCup = IsThirtyCarCupActive();
    const bool expandedSingleRace = !expandedCup && state.gridApplied;
    const bool fixedOpponentCup = IsCupWithFixedOpponents();

    if (!expandedSingleRace && !expandedCup && !fixedOpponentCup) {
        return;
    }

    // Expanded single races/knockouts may spawn cars with runtime IDs that do
    // not match their participant indexes; cups retain index-based IDs.
    if (expandedSingleRace) {
        gridCarIds.reserve(randomizerMaxCarCount);
        for (const int runtimeCarId : state.runtimeCarIds) {
            if (runtimeCarId >= 0) {
                gridCarIds.push_back(runtimeCarId);
            }
        }

        const int targetCarCount = std::clamp(
            GetRandomizerContext().carState.carsPerRace,
            randomizerMinCarCount,
            randomizerMaxCarCount
        );
        if (gridCarIds.size() != static_cast<size_t>(targetCarCount)) {
            return;
        }
    }
    else {
        const int participantCount = GetParticipantCount();
        gridCarIds.reserve(participantCount);
        for (int carId = 0; carId < participantCount; ++carId) {
            gridCarIds.push_back(carId);
        }
    }

    if ((expandedSingleRace || expandedCup) && gridCarIds.size() > 1) {
        // ApplyThirtyCarGrid/ApplyThirtyCarCupGrid exchanged the physical
        // first and final slots to put the player at the back. Keep the same
        // order here because runtime IDs remain in participant order.
        std::swap(gridCarIds.front(), gridCarIds.back());
    }

    const int playerCarId = expandedSingleRace ? state.runtimeCarIds[0] : 0;
    ShuffleGridCars(gridCarIds, playerCarId);
}

} // namespace Randomizer
