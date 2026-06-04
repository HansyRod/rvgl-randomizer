#pragma once
#include "RVGLStructs.h"
#include <vector>

namespace Randomizer {

int GetParticipantCount();
int GetTotalCarModelCount();
Vec3 GetCarPos(int carId);
void SetCarPos(int carId, const Vec3& pos);
int GetCarModelRating(int modelId);
bool IsCarModelCpuSelectable(int modelId);
int PickRandomFromPool(const std::vector<int>& pool);

void ApplyThirtyCarGrid();
void ExpandRaceParticipantsToThirty();
void MovePlayersToBackAfterRacePositions();
void ResetThirtyCarModState();

}
