#pragma once

#include <cstdint>
#include <cstdio>

namespace Randomizer {

using FnCup_GenerateOpponentGrid = void(*)();
using FnBuildGrid = void(*)();
using FnUpdateCupPostRaceProgress = void(*)();
using FnDrawCupProgressMessage = void(*)();

extern FnCup_GenerateOpponentGrid Orig_Cup_GenerateOpponentGrid;
extern FnBuildGrid Orig_BuildGrid;
extern FnUpdateCupPostRaceProgress Orig_UpdateCupPostRaceProgress;
extern FnDrawCupProgressMessage Orig_DrawCupProgressMessage;

bool IsThirtyCarCupActive();
void ResetThirtyCarCupState();
void ApplyThirtyCarCupGrid();
void PrepareThirtyCarCupStageFinished();
bool HandleThirtyCarCupOnStageFinished(uint64_t param1, uint64_t param3, FILE* file);

void Hook_Cup_GenerateOpponentGrid();
void Hook_BuildGrid();
void Hook_UpdateCupPostRaceProgress();
void Hook_DrawCupProgressMessage();

} // namespace Randomizer
