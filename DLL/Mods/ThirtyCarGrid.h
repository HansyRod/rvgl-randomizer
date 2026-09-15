#pragma once

#include <array>

#include "RandomizerState.h"
#include "RVGLStructs.h"

namespace Randomizer {

// Builds the expanded starting grid from RVGL's authored sixteen-car grid.
// The implementation validates each candidate against the collision grid,
// track route zones, floor geometry, footprint clearance, and row spacing.
bool CalculateThirtyCarGridPositions(
    int existingCarCount,
    int targetCarCount,
    std::array<Vec3, randomizerMaxCarCount>& outPositions,
    std::array<Vec3, randomizerMaxCarCount>& outForwardDirections
);

}
