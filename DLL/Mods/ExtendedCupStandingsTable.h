#pragma once

#include "ExtendedCupResults.h"

namespace Randomizer {

void DrawExtendedCupStandingsTable(
    bool active,
    CupProfile* activeCup,
    const ExtendedCupResultsState& results,
    const char* playerName
);

} // namespace Randomizer
