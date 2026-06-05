#pragma once

#include <cstdint>
#include <cstdio>

namespace Randomizer {

using FnCup_GenerateOpponentGrid = void(*)();
using FnBuildGrid = void(*)();
using FnUpdateCupPostRaceProgress = void(*)();
using FnDrawCupStandingsTable = void(*)();

extern FnCup_GenerateOpponentGrid Orig_Cup_GenerateOpponentGrid;
extern FnBuildGrid Orig_BuildGrid;
extern FnUpdateCupPostRaceProgress Orig_UpdateCupPostRaceProgress;
extern FnDrawCupStandingsTable Orig_DrawCupStandingsTable;

bool IsThirtyCarCupActive();
void ResetThirtyCarCupState();
void ApplyThirtyCarCupGrid();
void MoveThirtyCarCupPlayerToBackAfterRacePositions();
void PrepareThirtyCarCupStageFinished();
bool HandleThirtyCarCupOnStageFinished();

void Hook_Cup_GenerateOpponentGrid();
void Hook_BuildGrid();
void Hook_UpdateCupPostRaceProgress();
void Hook_DrawCupStandingsTable();

} // namespace Randomizer
