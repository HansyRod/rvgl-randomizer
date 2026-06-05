#pragma once

#include <cstdint>
#include <cstdio>

struct CupProfile;

namespace Randomizer {

using FnBuildGrid = void(*)();
using FnUpdateCupPostRaceProgress = void(*)();
using FnDrawCupStandingsTable = void(*)();

extern FnBuildGrid Orig_BuildGrid;
extern FnUpdateCupPostRaceProgress Orig_UpdateCupPostRaceProgress;
extern FnDrawCupStandingsTable Orig_DrawCupStandingsTable;

struct ExtendedCupResultsState;
struct RandomizedCup;

bool IsThirtyCarCupActive();
void ResetThirtyCarCupState();
void StartThirtyCarCupState(
    int selectedCupIndex,
    CupProfile* cup,
    const RandomizedCup* cupConfig,
    const ExtendedCupResultsState& results
);
void ApplyThirtyCarCupGrid();
void MoveThirtyCarCupPlayerToBackAfterRacePositions();
void PrepareThirtyCarCupStageFinished();
bool HandleThirtyCarCupOnStageFinished();

void Hook_BuildGrid();
void Hook_UpdateCupPostRaceProgress();
void Hook_DrawCupStandingsTable();

} // namespace Randomizer
