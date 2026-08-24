#pragma once
#include "RandomizerState.h"
#include "RVGLStructs.h"
#include <array>
#include <vector>

namespace Randomizer {

int GetParticipantCount();
int GetTotalCarModelCount();
Vec3 GetCarPos(int carId);
void SetCarPos(int carId, const Vec3& pos);
void SetCarPosAndForwardDirection(int carId, const Vec3& pos, const Vec3& forwardDirection);
int GetCarModelRating(int modelId);
bool IsCarModelCpuSelectable(int modelId);
int PickRandomFromPool(const std::vector<int>& pool);
void ApplyThirtyCarGrid();
void ExpandRaceParticipantsToThirty();
bool MoveRuntimeCarsToBackAfterRacePositions(
    const std::array<int, randomizerMaxCarCount>& runtimeCarIds,
    int targetCarCount
);
void MovePlayersToBackAfterRacePositions();
void ResetThirtyCarPlayerPositionState();
void ResetThirtyCarModState();

}
