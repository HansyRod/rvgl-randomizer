#pragma once

#include "ExtendedCupResults.h"

struct CupProfile;

namespace Randomizer {

struct RandomizedCup;

using FnCup_GenerateOpponentGrid = void(*)();

extern FnCup_GenerateOpponentGrid Orig_Cup_GenerateOpponentGrid;

bool IsExtendedCupOpponentGrid(CupProfile* cup);
void Hook_Cup_GenerateOpponentGrid();

} // namespace Randomizer
