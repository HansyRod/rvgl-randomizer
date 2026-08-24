#include "ThirtyCarGrid.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

#include "Addresses.h"
#include "Logger.h"
#include "RVGLFunctions.h"

namespace Randomizer {

namespace {

// Grid-generation pipeline:
//   1. Read RVGL's authored sixteen-car grid.
//   2. Establish its coordinate system and native row layout.
//   3. Sample that corridor at the fixed 150-unit spacing.
//   4. Validate each lane with floor, route, footprint, and collision checks.
//   5. Continue along the route graph after the native corridor ends.
//   6. Assign straight-line or predecessor-facing headings around turns.
//
// CalculateThirtyCarGridPositions below only coordinates these stages; the
// implementation of each stage lives in its own named helper.

// The former 5x6 grid used 150 units between cars. This remains the target
// spacing for the expanded grid.
constexpr float kMinimumSpawnSeparation = 150.0f;

// Ground and floor validation.
constexpr float kGroundProbeHeightAbove = 128.0f;
constexpr float kGroundProbeDepthBelow = 512.0f;
constexpr float kMaximumFloorNormalY = -0.5f;
constexpr float kMaximumFootprintHeightDelta = 80.0f;

// Vertical and horizontal car-clearance probes.
constexpr float kSpawnClearanceHeight = 120.0f;
constexpr float kClearanceRayStartHeight = 20.0f;
constexpr std::array<float, 3> kLowerBodyProbeHeights = {
    { -60.0f, -25.0f, -5.0f }
};
constexpr std::array<float, 3> kHorizontalProbeHeights = {
    { 10.0f, 45.0f, 80.0f }
};
constexpr float kFootprintHalfWidth = 45.0f;
constexpr float kCarClearanceHalfWidth = 60.0f;
constexpr float kCarClearanceHalfLength = 90.0f;
constexpr float kFrontClearanceDistance = 100.0f;
constexpr float kNativeCorridorFrontClearanceDistance = 60.0f;
constexpr float kSideClearanceDistance = 25.0f;

// Route and comparison tolerances.
constexpr float kTraceHitEpsilon = 0.0001f;
constexpr float kGridRowSearchStep = 25.0f;
constexpr int kMaxGridRowSearchAttempts = 24;
constexpr int kFullWidthSpacingRetryAttempts = 4;
constexpr float kPredecessorFacingTurnThreshold = 0.95f;
// Route distance follows the path, while candidate spacing is measured as a
// straight-line X/Z distance. A small tolerance accounts for curved paths.
constexpr float kSpawnDistanceComparisonTolerance = 1.0f;

struct NativeGridBasis {
    Vec3 nativeStart = {};
    Vec3 right = {};
    Vec3 look = {};
    Vec3 back = {};
};

struct GridRow {
    std::vector<Vec3> positions;
    Vec3 center = {};
    float along = 0.0f;
    Vec3 forwardDirection = {};
};

struct NativeGridLayout {
    std::vector<GridRow> rows;
    std::vector<float> centerDistances;
    float corridorLength = 0.0f;
    size_t maximumLaneCount = 0;
};

struct ProjectedStart {
    float along = 0.0f;
    Vec3 position = {};
    Vec3 forwardDirection = {};
};

struct LaneOffsetCandidate {
    std::vector<float> offsets;
    float score = 0.0f;
};

struct GeneratedGridRow {
    int firstPositionIndex = 0;
    int positionCount = 0;
    Vec3 forwardDirection = {};
    std::vector<float> laneOffsets;
};

struct RouteTraversalState {
    RouteSectionRuntime* sections = nullptr;
    int sectionCount = 0;
    Vec3 center = {};
    Vec3 direction = {};
    Vec3 right = {};
    int currentSectionIndex = 0;
    int previousSectionIndex = -1;
    int nextSectionIndex = -1;
    float traversedDistance = 0.0f;
    bool initialized = false;
};

float DotXZ(const Vec3& first, const Vec3& second) {
    return first.x * second.x + first.z * second.z;
}

bool NormalizeXZ(Vec3& vector) {
    const float length = std::sqrt(DotXZ(vector, vector));
    if (length <= kTraceHitEpsilon) {
        return false;
    }

    vector.x /= length;
    vector.z /= length;
    return true;
}

int CountSetBits(unsigned int value) {
    int bitCount = 0;
    while (value != 0) {
        value &= value - 1;
        ++bitCount;
    }
    return bitCount;
}

// Emits the one diagnostic that is useful in normal builds: why generation
// could not produce a complete grid.
bool ReportGridFailure(const char* failureReason) {
    Logger::TimestampLogf(
        "[ThirtyCarGrid] CALCULATION FAILED: %s",
        failureReason != nullptr ? failureReason : "unknown reason"
    );
    return false;
}

bool ValidateGridInputs(
    int existingCarCount,
    int targetCarCount,
    const char*& failureReason
) {
    if (existingCarCount <= 0 ||
        targetCarCount <= 0 ||
        targetCarCount > randomizerMaxCarCount ||
        existingCarCount > targetCarCount) {
        failureReason = "invalid participant or target count";
        return false;
    }

    if (existingCarCount < vanillaMaxCarCount) {
        failureReason = "fewer than sixteen native cars exist";
        return false;
    }

    if (RVGL_ComputeSpawnOrientation == nullptr ||
        RVGL_GetCollisionGridCellIndex == nullptr ||
        RVGL_TraceSegmentAgainstCollisionGrid == nullptr ||
        RVGL_FindTrackZoneForCarBruteForce == nullptr) {
        failureReason = "one or more native collision functions are unavailable";
        return false;
    }

    return true;
}

// Reads RVGL's authored sixteen-car start slots. These positions establish the
// safe starting envelope; they are not copied directly into the 30-car grid.
bool LoadAuthoredStartGrid(
    std::array<Vec3, vanillaMaxCarCount>& authoredPositions,
    std::array<Vec3, vanillaMaxCarCount>& authoredForwardDirections,
    const char*& failureReason
) {
    for (int startSlot = 0; startSlot < vanillaMaxCarCount; ++startSlot) {
        float slotPositionRaw[3] = {};
        float slotOrientation[12] = {};
        RVGL_ComputeSpawnOrientation(startSlot, slotPositionRaw, slotOrientation);

        const Vec3 position{
            slotPositionRaw[0],
            slotPositionRaw[1],
            slotPositionRaw[2],
        };
        if (!std::isfinite(position.x) ||
            !std::isfinite(position.y) ||
            !std::isfinite(position.z)) {
            failureReason = "an authored start-slot position is non-finite";
            return false;
        }

        authoredPositions[startSlot] = position;
        authoredForwardDirections[startSlot] = {
            slotOrientation[6],
            0.0f,
            slotOrientation[8],
        };
    }

    return true;
}

// Establishes the horizontal coordinate system used by all grid calculations.
bool EstablishGridBasis(
    const std::array<Vec3, vanillaMaxCarCount>& authoredPositions,
    NativeGridBasis& basis,
    const char*& failureReason
) {
    float nativeStartRaw[3] = {};
    float nativeOrientation[12] = {};
    RVGL_ComputeSpawnOrientation(0, nativeStartRaw, nativeOrientation);

    basis.nativeStart = {
        nativeStartRaw[0],
        nativeStartRaw[1],
        nativeStartRaw[2],
    };
    basis.right = {
        nativeOrientation[0],
        0.0f,
        nativeOrientation[2],
    };
    basis.look = {
        nativeOrientation[6],
        0.0f,
        nativeOrientation[8],
    };

    if (!std::isfinite(basis.nativeStart.x) ||
        !std::isfinite(basis.nativeStart.y) ||
        !std::isfinite(basis.nativeStart.z) ||
        !NormalizeXZ(basis.right) ||
        !NormalizeXZ(basis.look)) {
        failureReason = "native spawn orientation has an invalid horizontal basis";
        return false;
    }

    // The authored slot and the orientation query should describe the same
    // starting point. Use the authored point if the binary exposes a minor
    // representation difference between the two reads.
    basis.nativeStart = authoredPositions[0];
    basis.back = {
        -basis.look.x,
        0.0f,
        -basis.look.z,
    };
    return true;
}

// Groups authored positions into native rows by their distance along the
// initial backward axis. This supports native grids with varying lane counts.
bool GroupNativeRows(
    const std::array<Vec3, vanillaMaxCarCount>& authoredPositions,
    const std::array<Vec3, vanillaMaxCarCount>& authoredForwardDirections,
    const NativeGridBasis& basis,
    std::vector<GridRow>& rows,
    const char*& failureReason
) {
    std::array<ProjectedStart, vanillaMaxCarCount> projectedStarts = {};

    for (int startSlot = 0; startSlot < vanillaMaxCarCount; ++startSlot) {
        const Vec3 relative{
            authoredPositions[startSlot].x - basis.nativeStart.x,
            0.0f,
            authoredPositions[startSlot].z - basis.nativeStart.z,
        };
        Vec3 forwardDirection = authoredForwardDirections[startSlot];
        if (!NormalizeXZ(forwardDirection)) {
            failureReason = "an authored start-slot orientation has no horizontal direction";
            return false;
        }

        if (DotXZ(forwardDirection, basis.look) < 0.0f) {
            forwardDirection.x = -forwardDirection.x;
            forwardDirection.z = -forwardDirection.z;
        }

        projectedStarts[startSlot] = {
            DotXZ(relative, basis.back),
            authoredPositions[startSlot],
            forwardDirection,
        };
    }

    std::sort(
        projectedStarts.begin(),
        projectedStarts.end(),
        [](const ProjectedStart& first, const ProjectedStart& second) {
            return first.along < second.along;
        }
    );

    float smallestGap = (std::numeric_limits<float>::max)();
    float largestGap = 0.0f;
    for (int index = 1; index < vanillaMaxCarCount; ++index) {
        const float gap =
            projectedStarts[index].along - projectedStarts[index - 1].along;
        if (gap > kTraceHitEpsilon) {
            smallestGap = (std::min)(smallestGap, gap);
            largestGap = (std::max)(largestGap, gap);
        }
    }

    if (largestGap <= kTraceHitEpsilon) {
        failureReason = "native positions have no usable row spacing";
        return false;
    }

    float rowGapThreshold = largestGap * 0.5f;
    if (smallestGap < largestGap / 1.5f) {
        rowGapThreshold = (smallestGap + largestGap) * 0.5f;
    }
    rowGapThreshold =
        (std::max)(rowGapThreshold, kMinimumSpawnSeparation * 0.5f);

    rows.clear();
    rows.push_back({});
    float previousAlong = projectedStarts[0].along;

    for (const ProjectedStart& projectedStart : projectedStarts) {
        if (!rows.back().positions.empty() &&
            projectedStart.along - previousAlong > rowGapThreshold) {
            rows.push_back({});
        }

        rows.back().positions.push_back(projectedStart.position);
        rows.back().forwardDirection.x += projectedStart.forwardDirection.x;
        rows.back().forwardDirection.z += projectedStart.forwardDirection.z;
        previousAlong = projectedStart.along;
    }

    if (rows.size() < 2) {
        failureReason =
            "native positions could not be grouped into at least two rows";
        return false;
    }

    for (GridRow& row : rows) {
        for (const Vec3& position : row.positions) {
            row.center.x += position.x;
            row.center.y += position.y;
            row.center.z += position.z;
        }

        const float inverseCount =
            1.0f / static_cast<float>(row.positions.size());
        row.center.x *= inverseCount;
        row.center.y *= inverseCount;
        row.center.z *= inverseCount;

        if (!NormalizeXZ(row.forwardDirection)) {
            failureReason = "a native row has no usable authored heading";
            return false;
        }

        const Vec3 relative{
            row.center.x - basis.nativeStart.x,
            0.0f,
            row.center.z - basis.nativeStart.z,
        };
        row.along = DotXZ(relative, basis.back);
    }

    std::sort(
        rows.begin(),
        rows.end(),
        [](const GridRow& first, const GridRow& second) {
            return first.along < second.along;
        }
    );
    return true;
}

// Measures the authored corridor so interpolated rows can be placed at the
// desired 150-unit spacing.
bool BuildNativeCorridorDistances(
    const std::vector<GridRow>& rows,
    NativeGridLayout& layout,
    const char*& failureReason
) {
    layout.centerDistances.assign(rows.size(), 0.0f);

    for (size_t rowIndex = 1; rowIndex < rows.size(); ++rowIndex) {
        const float deltaX =
            rows[rowIndex].center.x - rows[rowIndex - 1].center.x;
        const float deltaZ =
            rows[rowIndex].center.z - rows[rowIndex - 1].center.z;
        const float segmentLength = std::sqrt(deltaX * deltaX + deltaZ * deltaZ);
        if (segmentLength <= kTraceHitEpsilon) {
            failureReason = "two native row centers have no horizontal separation";
            return false;
        }
        layout.centerDistances[rowIndex] =
            layout.centerDistances[rowIndex - 1] + segmentLength;
    }

    layout.rows = rows;
    layout.corridorLength = layout.centerDistances.back();
    layout.maximumLaneCount = (std::max_element)(
        rows.begin(),
        rows.end(),
        [](const GridRow& first, const GridRow& second) {
            return first.positions.size() < second.positions.size();
        }
    )->positions.size();

    if (layout.maximumLaneCount == 0) {
        failureReason = "native rows have no lanes";
        return false;
    }

    return true;
}

// Finds the collision floor below a world-space X/Z coordinate.
bool FindGround(
    float x,
    float z,
    float referenceY,
    float& groundY
) {
    Vec3 gridPoint{ x, referenceY, z };
    if (RVGL_GetCollisionGridCellIndex(&gridPoint) < 0) {
        return false;
    }

    Vec3 traceStart{ x, referenceY - kGroundProbeHeightAbove, z };
    Vec3 traceEnd{ x, referenceY + kGroundProbeDepthBelow, z };
    float hitFraction = 1.0f;
    CollisionPlane* hitPlane = nullptr;
    RVGL_TraceSegmentAgainstCollisionGrid(
        &traceStart,
        &traceEnd,
        &hitFraction,
        &hitPlane
    );

    if (hitFraction < kTraceHitEpsilon ||
        hitFraction >= 1.0f ||
        hitPlane == nullptr ||
        hitPlane->normalY > kMaximumFloorNormalY) {
        return false;
    }

    groundY = traceStart.y +
        (traceEnd.y - traceStart.y) * hitFraction;
    return std::isfinite(groundY);
}

// Checks the vertical volume above a candidate car.
bool HasVerticalClearance(
    const Vec3& candidate,
    float nativeHeightAboveGround
) {
    const float startHeightOffset =
        kClearanceRayStartHeight - nativeHeightAboveGround;
    Vec3 traceStart{
        candidate.x,
        candidate.y + startHeightOffset,
        candidate.z,
    };
    Vec3 traceEnd{
        traceStart.x,
        traceStart.y + kSpawnClearanceHeight,
        traceStart.z,
    };
    float hitFraction = 1.0f;
    CollisionPlane* hitPlane = nullptr;
    RVGL_TraceSegmentAgainstCollisionGrid(
        &traceStart,
        &traceEnd,
        &hitFraction,
        &hitPlane
    );
    return hitFraction >= 1.0f;
}

// Traces one segment through RVGL's static and dynamic collision geometry.
// Near-horizontal surfaces are ignored because floor, ramps, and similar
// geometry are validated by the ground and footprint checks instead.
bool IsCollisionSegmentClear(
    const Vec3& traceStart,
    const Vec3& traceEnd,
    bool ignoreNearHorizontalSurface
) {
    Vec3 mutableTraceStart = traceStart;
    Vec3 mutableTraceEnd = traceEnd;
    float hitFraction = 1.0f;
    CollisionPlane* hitPlane = nullptr;
    RVGL_TraceSegmentAgainstCollisionGrid(
        &mutableTraceStart,
        &mutableTraceEnd,
        &hitFraction,
        &hitPlane
    );

    if (hitFraction >= 1.0f || hitPlane == nullptr) {
        return true;
    }

    if (ignoreNearHorizontalSurface &&
        std::fabs(hitPlane->normalY) >= 0.85f) {
        return true;
    }
    return false;
}

// Checks a candidate's body volume using multiple horizontal lines at several
// heights. This approximates the car's occupied volume while keeping the
// native segment tracer as the collision primitive.
bool HasHorizontalSpawnClearance(
    const Vec3& candidate,
    const Vec3& lateralAxis,
    const Vec3& forwardAxis,
    float nativeHeightAboveGround,
    float frontClearanceDistance
) {
    const float lateralProbeHalfWidth =
        kCarClearanceHalfWidth + kSideClearanceDistance;
    const std::array<float, 9> lateralSamples = {{
        -lateralProbeHalfWidth,
        -lateralProbeHalfWidth * 0.75f,
        -lateralProbeHalfWidth * 0.5f,
        -lateralProbeHalfWidth * 0.25f,
        0.0f,
        lateralProbeHalfWidth * 0.25f,
        lateralProbeHalfWidth * 0.5f,
        lateralProbeHalfWidth * 0.75f,
        lateralProbeHalfWidth,
    }};
    const std::array<float, 7> longitudinalSamples = {{
        -kCarClearanceHalfLength,
        -kCarClearanceHalfLength * 0.666667f,
        -kCarClearanceHalfLength * 0.333333f,
        0.0f,
        kCarClearanceHalfLength * 0.333333f,
        kCarClearanceHalfLength * 0.666667f,
        kCarClearanceHalfLength,
    }};

    std::array<float, 6> bodyHeights = {{
        kLowerBodyProbeHeights[0],
        kLowerBodyProbeHeights[1],
        kLowerBodyProbeHeights[2],
        kHorizontalProbeHeights[0] - nativeHeightAboveGround,
        kHorizontalProbeHeights[1] - nativeHeightAboveGround,
        kHorizontalProbeHeights[2] - nativeHeightAboveGround,
    }};

    for (float heightOffset : bodyHeights) {
        for (float lateralOffset : lateralSamples) {
            const Vec3 traceStart{
                candidate.x - forwardAxis.x * kCarClearanceHalfLength +
                    lateralAxis.x * lateralOffset,
                candidate.y + heightOffset,
                candidate.z - forwardAxis.z * kCarClearanceHalfLength +
                    lateralAxis.z * lateralOffset,
            };
            const Vec3 traceEnd{
                candidate.x + forwardAxis.x *
                        (kCarClearanceHalfLength + frontClearanceDistance) +
                    lateralAxis.x * lateralOffset,
                candidate.y + heightOffset,
                candidate.z + forwardAxis.z *
                        (kCarClearanceHalfLength + frontClearanceDistance) +
                    lateralAxis.z * lateralOffset,
            };

            if (!IsCollisionSegmentClear(traceStart, traceEnd, true)) {
                return false;
            }
        }

        for (float longitudinalOffset : longitudinalSamples) {
            const Vec3 traceStart{
                candidate.x + forwardAxis.x * longitudinalOffset -
                    lateralAxis.x * (kCarClearanceHalfWidth + kSideClearanceDistance),
                candidate.y + heightOffset,
                candidate.z + forwardAxis.z * longitudinalOffset -
                    lateralAxis.z * (kCarClearanceHalfWidth + kSideClearanceDistance),
            };
            const Vec3 traceEnd{
                candidate.x + forwardAxis.x * longitudinalOffset +
                    lateralAxis.x * (kCarClearanceHalfWidth + kSideClearanceDistance),
                candidate.y + heightOffset,
                candidate.z + forwardAxis.z * longitudinalOffset +
                    lateralAxis.z * (kCarClearanceHalfWidth + kSideClearanceDistance),
            };

            if (!IsCollisionSegmentClear(traceStart, traceEnd, true)) {
                return false;
            }
        }
    }

    return true;
}

// Uses RVGL's native route-zone lookup to reject positions outside the
// drivable area.
bool IsInsideTrackRoute(const Vec3& point) {
    TrackZoneQueryContext query = {};
    query.worldPosition = point;
    return RVGL_FindTrackZoneForCarBruteForce(&query);
}

// Validates and commits one candidate position. A candidate is not written to
// the output arrays until its floor, route, footprint, vertical, horizontal,
// and spacing checks all pass.
bool TryAddGridPoint(
    const Vec3& desiredPosition,
    const Vec3& lateralAxis,
    const Vec3& longitudinalAxis,
    Vec3 forwardDirection,
    float nativeHeightAboveGround,
    float frontClearanceDistance,
    int& chosenCount,
    std::array<Vec3, randomizerMaxCarCount>& outPositions,
    std::array<Vec3, randomizerMaxCarCount>& outForwardDirections,
    bool& rejectedForSpacing
) {
    rejectedForSpacing = false;

    float groundY = 0.0f;
    if (!FindGround(
            desiredPosition.x,
            desiredPosition.z,
            desiredPosition.y,
            groundY)) {
        return false;
    }

    const Vec3 candidate{
        desiredPosition.x,
        groundY + nativeHeightAboveGround,
        desiredPosition.z,
    };
    if (!IsInsideTrackRoute(candidate)) {
        return false;
    }

    const std::array<Vec3, 4> footprintOffsets = {{
        { lateralAxis.x * kFootprintHalfWidth, 0.0f,
          lateralAxis.z * kFootprintHalfWidth },
        { -lateralAxis.x * kFootprintHalfWidth, 0.0f,
          -lateralAxis.z * kFootprintHalfWidth },
        { longitudinalAxis.x * kFootprintHalfWidth, 0.0f,
          longitudinalAxis.z * kFootprintHalfWidth },
        { -longitudinalAxis.x * kFootprintHalfWidth, 0.0f,
          -longitudinalAxis.z * kFootprintHalfWidth },
    }};

    for (const Vec3& offset : footprintOffsets) {
        float footprintGroundY = 0.0f;
        if (!FindGround(
                candidate.x + offset.x,
                candidate.z + offset.z,
                desiredPosition.y,
                footprintGroundY)) {
            return false;
        }
        if (std::fabs(footprintGroundY - groundY) >
            kMaximumFootprintHeightDelta) {
            return false;
        }

        const Vec3 footprintSpawnPoint{
            candidate.x + offset.x,
            footprintGroundY + nativeHeightAboveGround,
            candidate.z + offset.z,
        };
        if (!IsInsideTrackRoute(footprintSpawnPoint)) {
            return false;
        }
    }

    if (!HasVerticalClearance(candidate, nativeHeightAboveGround)) {
        return false;
    }
    if (!NormalizeXZ(forwardDirection)) {
        return false;
    }
    if (!HasHorizontalSpawnClearance(
            candidate,
            lateralAxis,
            forwardDirection,
            nativeHeightAboveGround,
            frontClearanceDistance)) {
        return false;
    }

    const float minimumDistance =
        kMinimumSpawnSeparation - kSpawnDistanceComparisonTolerance;
    const float minimumDistanceSquared = minimumDistance * minimumDistance;
    for (int index = 0; index < chosenCount; ++index) {
        const float deltaX = candidate.x - outPositions[index].x;
        const float deltaZ = candidate.z - outPositions[index].z;
        if (deltaX * deltaX + deltaZ * deltaZ < minimumDistanceSquared) {
            rejectedForSpacing = true;
            return false;
        }
    }

    outPositions[chosenCount] = candidate;
    outForwardDirections[chosenCount] = forwardDirection;
    ++chosenCount;
    return true;
}

// Interpolates a position and heading between two authored native rows.
bool SampleNativeCorridor(
    const NativeGridLayout& layout,
    float distance,
    Vec3& center,
    Vec3& trailingDirection
) {
    if (distance < -kTraceHitEpsilon ||
        distance > layout.corridorLength + kTraceHitEpsilon) {
        return false;
    }

    size_t segmentIndex = 1;
    while (segmentIndex < layout.centerDistances.size() &&
           distance > layout.centerDistances[segmentIndex]) {
        ++segmentIndex;
    }
    if (segmentIndex >= layout.centerDistances.size()) {
        segmentIndex = layout.centerDistances.size() - 1;
    }

    const float segmentStartDistance =
        layout.centerDistances[segmentIndex - 1];
    const float segmentLength =
        layout.centerDistances[segmentIndex] - segmentStartDistance;
    const float interpolation = segmentLength <= kTraceHitEpsilon
        ? 0.0f
        : (std::clamp)(
            (distance - segmentStartDistance) / segmentLength,
            0.0f,
            1.0f
        );

    const Vec3& start = layout.rows[segmentIndex - 1].center;
    const Vec3& end = layout.rows[segmentIndex].center;
    center = {
        start.x + (end.x - start.x) * interpolation,
        start.y + (end.y - start.y) * interpolation,
        start.z + (end.z - start.z) * interpolation,
    };

    Vec3 interpolatedForwardDirection{
        layout.rows[segmentIndex - 1].forwardDirection.x +
            (layout.rows[segmentIndex].forwardDirection.x -
             layout.rows[segmentIndex - 1].forwardDirection.x) * interpolation,
        0.0f,
        layout.rows[segmentIndex - 1].forwardDirection.z +
            (layout.rows[segmentIndex].forwardDirection.z -
             layout.rows[segmentIndex - 1].forwardDirection.z) * interpolation,
    };
    if (!NormalizeXZ(interpolatedForwardDirection)) {
        return false;
    }

    trailingDirection = {
        -interpolatedForwardDirection.x,
        0.0f,
        -interpolatedForwardDirection.z,
    };
    return true;
}

// Converts a route-section pointer into its index within RVGL's route array.
int GetRouteSectionIndex(
    const RouteTraversalState& route,
    const RouteSectionRuntime* section
) {
    if (section == nullptr || route.sections == nullptr || route.sectionCount <= 0) {
        return -1;
    }

    const uintptr_t firstAddress =
        reinterpret_cast<uintptr_t>(route.sections);
    const uintptr_t sectionAddress =
        reinterpret_cast<uintptr_t>(section);
    const uintptr_t routeBytes =
        static_cast<uintptr_t>(route.sectionCount) *
        sizeof(RouteSectionRuntime);

    if (sectionAddress < firstAddress ||
        sectionAddress >= firstAddress + routeBytes ||
        (sectionAddress - firstAddress) % sizeof(RouteSectionRuntime) != 0) {
        return -1;
    }

    return static_cast<int>(
        (sectionAddress - firstAddress) / sizeof(RouteSectionRuntime)
    );
}

// Chooses the next route section whose direction best matches travel.
int FindFollowingRouteSection(
    const RouteTraversalState& route,
    int routeIndex,
    int previousRouteIndex,
    const Vec3& desiredDirection
) {
    const RouteSectionRuntime& section = route.sections[routeIndex];
    std::array<RouteSectionRuntime*, 8> links = {};

    for (int linkIndex = 0; linkIndex < 4; ++linkIndex) {
        links[linkIndex] = section.nextSections[linkIndex];
        links[linkIndex + 4] = section.previousSections[linkIndex];
    }

    int bestRouteIndex = -1;
    float bestAlignment = -(std::numeric_limits<float>::max)();
    for (RouteSectionRuntime* link : links) {
        const int linkIndex = GetRouteSectionIndex(route, link);
        if (linkIndex < 0 || linkIndex == previousRouteIndex) {
            continue;
        }

        Vec3 linkDirection{
            route.sections[linkIndex].position.x - section.position.x,
            0.0f,
            route.sections[linkIndex].position.z - section.position.z,
        };
        if (!NormalizeXZ(linkDirection)) {
            continue;
        }

        const float alignment = DotXZ(linkDirection, desiredDirection);
        if (alignment > bestAlignment) {
            bestAlignment = alignment;
            bestRouteIndex = linkIndex;
        }
    }

    return bestRouteIndex;
}

// Returns the route graph RVGL loaded for the active track.
RouteSectionRuntime* GetLoadedRouteSections() {
    return *reinterpret_cast<RouteSectionRuntime**>(
        AbsFromRva(RVA_ROUTE_SECTIONS)
    );
}

int GetLoadedRouteSectionCount() {
    return *reinterpret_cast<int*>(AbsFromRva(RVA_ROUTE_SECTION_COUNT));
}

// Starts route traversal at the native corridor's last row. The route graph
// is used only after the authored grid has been exhausted, so it lets the
// generator continue around corners without inventing a straight line.
bool InitializeRouteTraversal(
    const NativeGridBasis& basis,
    RouteTraversalState& route,
    const char*& failureReason
) {
    route.sections = GetLoadedRouteSections();
    route.sectionCount = GetLoadedRouteSectionCount();
    if (route.sections == nullptr || route.sectionCount <= 0) {
        failureReason = "route-section graph is unavailable";
        return false;
    }

    float nearestDistanceSquared = (std::numeric_limits<float>::max)();
    route.currentSectionIndex = -1;
    for (int sectionIndex = 0;
         sectionIndex < route.sectionCount;
         ++sectionIndex) {
        const float deltaX =
            route.sections[sectionIndex].position.x - route.center.x;
        const float deltaZ =
            route.sections[sectionIndex].position.z - route.center.z;
        const float distanceSquared = deltaX * deltaX + deltaZ * deltaZ;
        if (distanceSquared < nearestDistanceSquared) {
            nearestDistanceSquared = distanceSquared;
            route.currentSectionIndex = sectionIndex;
        }
    }

    if (route.currentSectionIndex < 0) {
        failureReason = "route-section graph has no usable section";
        return false;
    }

    route.previousSectionIndex = -1;
    route.nextSectionIndex = FindFollowingRouteSection(
        route,
        route.currentSectionIndex,
        route.previousSectionIndex,
        route.direction
    );
    if (route.nextSectionIndex < 0) {
        failureReason = "route-section graph has no continuation";
        return false;
    }

    route.right = basis.right;
    route.initialized = true;
    return true;
}

// Advances through connected route sections by a measured distance.
bool AdvanceAlongRoute(
    float distance,
    RouteTraversalState& route,
    const char*& failureReason
) {
    float remainingDistance = distance;
    while (remainingDistance > kTraceHitEpsilon) {
        const Vec3& target =
            route.sections[route.nextSectionIndex].position;
        Vec3 segmentDirection{
            target.x - route.center.x,
            0.0f,
            target.z - route.center.z,
        };
        const float segmentLength =
            std::sqrt(DotXZ(segmentDirection, segmentDirection));

        if (segmentLength <= kTraceHitEpsilon) {
            route.previousSectionIndex = route.currentSectionIndex;
            route.currentSectionIndex = route.nextSectionIndex;
            route.nextSectionIndex = FindFollowingRouteSection(
                route,
                route.currentSectionIndex,
                route.previousSectionIndex,
                route.direction
            );
            if (route.nextSectionIndex < 0) {
                failureReason =
                    "route traversal ended without a connected next section";
                return false;
            }
            continue;
        }

        NormalizeXZ(segmentDirection);
        const float travelDistance =
            (std::min)(remainingDistance, segmentLength);
        const float interpolation = travelDistance / segmentLength;

        route.center.x += segmentDirection.x * travelDistance;
        route.center.y +=
            (target.y - route.center.y) * interpolation;
        route.center.z += segmentDirection.z * travelDistance;
        route.direction = segmentDirection;
        remainingDistance -= travelDistance;

        if (travelDistance + kTraceHitEpsilon >= segmentLength) {
            route.previousSectionIndex = route.currentSectionIndex;
            route.currentSectionIndex = route.nextSectionIndex;
            route.nextSectionIndex = FindFollowingRouteSection(
                route,
                route.currentSectionIndex,
                route.previousSectionIndex,
                route.direction
            );
            if (route.nextSectionIndex < 0 &&
                remainingDistance > kTraceHitEpsilon) {
                failureReason =
                    "route traversal ended without a connected next section";
                return false;
            }
        }
    }

    return true;
}

// Builds lane offsets for a row. Full rows use the centered 150-unit lattice;
// narrower rows reuse subsets of the preceding row's offsets so they may
// move around an obstacle without shifting every later row sideways.
std::vector<LaneOffsetCandidate> BuildLaneCandidates(
    int lanesToPlace,
    const std::vector<float>& previousLaneOffsets
) {
    std::vector<LaneOffsetCandidate> candidates;
    if (lanesToPlace <= 0) {
        return candidates;
    }

    if (previousLaneOffsets.empty()) {
        LaneOffsetCandidate centered;
        centered.offsets.reserve(lanesToPlace);
        const float centerOffset =
            0.5f * static_cast<float>(lanesToPlace - 1);
        for (int laneIndex = 0; laneIndex < lanesToPlace; ++laneIndex) {
            centered.offsets.push_back(
                (static_cast<float>(laneIndex) - centerOffset) *
                kMinimumSpawnSeparation
            );
        }
        candidates.push_back(centered);
        return candidates;
    }

    const unsigned int subsetCount =
        1u << static_cast<unsigned int>(previousLaneOffsets.size());
    for (unsigned int mask = 0; mask < subsetCount; ++mask) {
        if (CountSetBits(mask) != lanesToPlace) {
            continue;
        }

        LaneOffsetCandidate candidate;
        float offsetSum = 0.0f;
        for (size_t laneIndex = 0;
             laneIndex < previousLaneOffsets.size();
             ++laneIndex) {
            if ((mask & (1u << static_cast<unsigned int>(laneIndex))) == 0) {
                continue;
            }
            candidate.offsets.push_back(previousLaneOffsets[laneIndex]);
            offsetSum += previousLaneOffsets[laneIndex];
        }

        candidate.score = std::fabs(
            offsetSum / static_cast<float>(lanesToPlace)
        );
        for (size_t laneIndex = 1;
             laneIndex < candidate.offsets.size();
             ++laneIndex) {
            if (candidate.offsets[laneIndex] -
                    candidate.offsets[laneIndex - 1] >
                kMinimumSpawnSeparation +
                    kSpawnDistanceComparisonTolerance) {
                candidate.score += 10000.0f;
            }
        }
        candidates.push_back(candidate);
    }

    std::sort(
        candidates.begin(),
        candidates.end(),
        [](const LaneOffsetCandidate& first,
           const LaneOffsetCandidate& second) {
            return first.score < second.score;
        }
    );
    return candidates;
}

// Attempts rows from the native start toward the rear of the grid. Each row
// first tries to preserve the widest usable lane count, then narrows only when
// collision geometry requires it. Rows are searched in 25-unit increments so
// one rejected row cannot create a large artificial gap.
bool GenerateGridRows(
    int targetCarCount,
    const NativeGridLayout& layout,
    const NativeGridBasis& basis,
    float nativeHeightAboveGround,
    std::array<Vec3, randomizerMaxCarCount>& outPositions,
    std::array<Vec3, randomizerMaxCarCount>& outForwardDirections,
    std::vector<GeneratedGridRow>& generatedRows,
    const char*& failureReason
) {
    outPositions.fill({});
    outForwardDirections.fill({});
    generatedRows.clear();

    const int maximumCandidateLanes = (std::min)(
        targetCarCount,
        (std::max)(5, static_cast<int>(layout.maximumLaneCount))
    );

    RouteTraversalState route;
    route.center = layout.rows.back().center;
    route.direction = {
        layout.rows.back().center.x - layout.rows[layout.rows.size() - 2].center.x,
        0.0f,
        layout.rows.back().center.z - layout.rows[layout.rows.size() - 2].center.z,
    };
    if (!NormalizeXZ(route.direction)) {
        route.direction = basis.back;
    }
    route.right = basis.right;

    int chosenCount = 0;
    float lastAcceptedRowDistance = -kMinimumSpawnSeparation;
    int rowIndex = 0;

    while (chosenCount < targetCarCount) {
        const int maximumLanesThisRow = generatedRows.empty()
            ? maximumCandidateLanes
            : generatedRows.back().positionCount;
        const int chosenBeforeRow = chosenCount;
        float rowDistance = (std::max)(
            static_cast<float>(rowIndex) * kMinimumSpawnSeparation,
            lastAcceptedRowDistance + kMinimumSpawnSeparation
        );
        bool rowAccepted = false;

        for (int searchAttempt = 0;
             searchAttempt < kMaxGridRowSearchAttempts && !rowAccepted;
             ++searchAttempt) {
            Vec3 rowCenter = {};
            Vec3 trailingDirection = {};
            Vec3 rowRight = basis.right;
            bool isRouteExtension = false;

            if (rowDistance <= layout.corridorLength + kTraceHitEpsilon) {
                if (!SampleNativeCorridor(
                        layout,
                        rowDistance,
                        rowCenter,
                        trailingDirection)) {
                    rowDistance += kGridRowSearchStep;
                    continue;
                }
                rowRight = {
                    -trailingDirection.z,
                    0.0f,
                    trailingDirection.x,
                };
                if (DotXZ(rowRight, basis.right) < 0.0f) {
                    rowRight.x = -rowRight.x;
                    rowRight.z = -rowRight.z;
                }
            } else {
                if (!route.initialized &&
                    !InitializeRouteTraversal(
                        basis,
                        route,
                        failureReason)) {
                    return false;
                }

                const float extensionDistance =
                    rowDistance - layout.corridorLength;
                if (extensionDistance > route.traversedDistance +
                                            kTraceHitEpsilon &&
                    !AdvanceAlongRoute(
                        extensionDistance - route.traversedDistance,
                        route,
                        failureReason)) {
                    return false;
                }
                route.traversedDistance = extensionDistance;
                rowCenter = route.center;
                trailingDirection = route.direction;
                rowRight = route.right;
                isRouteExtension = true;
            }

            Vec3 forwardDirection{
                -trailingDirection.x,
                0.0f,
                -trailingDirection.z,
            };
            if (!NormalizeXZ(forwardDirection)) {
                rowDistance += kGridRowSearchStep;
                continue;
            }

            const int remainingCarCount = targetCarCount - chosenBeforeRow;
            const int maximumLanesForRemainingCars =
                (std::min)(maximumLanesThisRow, remainingCarCount);
            bool fullWidthSpacingRejected = false;

            for (int laneCount = maximumLanesForRemainingCars;
                 laneCount >= 1 && !rowAccepted;
                 --laneCount) {
                const std::vector<float> previousLaneOffsets =
                    generatedRows.empty()
                    ? std::vector<float>()
                    : generatedRows.back().laneOffsets;
                const std::vector<LaneOffsetCandidate> laneCandidates =
                    BuildLaneCandidates(laneCount, previousLaneOffsets);

                for (const LaneOffsetCandidate& laneCandidate : laneCandidates) {
                    chosenCount = chosenBeforeRow;
                    bool candidateAccepted = true;
                    bool candidateSpacingRejected = false;
                    for (float laneOffset : laneCandidate.offsets) {
                        const Vec3 desiredPosition{
                            rowCenter.x + rowRight.x * laneOffset,
                            rowCenter.y,
                            rowCenter.z + rowRight.z * laneOffset,
                        };
                        bool rejectedForSpacing = false;
                        if (!TryAddGridPoint(
                                desiredPosition,
                                rowRight,
                                trailingDirection,
                                forwardDirection,
                                nativeHeightAboveGround,
                                isRouteExtension
                                    ? kFrontClearanceDistance
                                    : kNativeCorridorFrontClearanceDistance,
                                chosenCount,
                                outPositions,
                                outForwardDirections,
                                rejectedForSpacing)) {
                            candidateAccepted = false;
                            candidateSpacingRejected |= rejectedForSpacing;
                            break;
                        }
                    }

                    if (candidateAccepted) {
                        generatedRows.push_back({
                            chosenBeforeRow,
                            laneCount,
                            forwardDirection,
                            laneCandidate.offsets,
                        });
                        lastAcceptedRowDistance = rowDistance;
                        rowAccepted = true;
                        break;
                    }

                    if (laneCount == maximumLanesForRemainingCars &&
                        candidateSpacingRejected) {
                        fullWidthSpacingRejected = true;
                    }
                }
            }

            // A full-width row rejected only by spacing can be a false
            // negative when the row center is still between two valid rows.
            // Search forward a little before reducing the lane count.
            if (!rowAccepted &&
                fullWidthSpacingRejected &&
                searchAttempt < kFullWidthSpacingRetryAttempts) {
                rowDistance += kGridRowSearchStep;
                continue;
            }

            if (!rowAccepted) {
                rowDistance += kGridRowSearchStep;
            }
        }

        if (!rowAccepted) {
            failureReason = "no valid position was found for a complete grid row";
            return false;
        }

        ++rowIndex;
    }

    return chosenCount == targetCarCount;
}

// Assigns headings after all positions are known. On a straight section the
// row keeps its sampled heading; around a turn each car points toward the
// closest car in the preceding row, matching the native grid's visual flow.
bool AssignGridHeadings(
    const std::vector<GeneratedGridRow>& generatedRows,
    const std::array<Vec3, randomizerMaxCarCount>& positions,
    std::array<Vec3, randomizerMaxCarCount>& forwardDirections,
    const char*& failureReason
) {
    for (size_t rowIndex = 1; rowIndex < generatedRows.size(); ++rowIndex) {
        const GeneratedGridRow& previousRow = generatedRows[rowIndex - 1];
        const GeneratedGridRow& currentRow = generatedRows[rowIndex];
        const float facingAlignment = DotXZ(
            previousRow.forwardDirection,
            currentRow.forwardDirection
        );

        if (facingAlignment >= kPredecessorFacingTurnThreshold) {
            for (int positionOffset = 0;
                 positionOffset < currentRow.positionCount;
                 ++positionOffset) {
                forwardDirections[
                    currentRow.firstPositionIndex + positionOffset
                ] = currentRow.forwardDirection;
            }
            continue;
        }

        for (int positionOffset = 0;
             positionOffset < currentRow.positionCount;
             ++positionOffset) {
            const int currentPositionIndex =
                currentRow.firstPositionIndex + positionOffset;
            float closestDistanceSquared =
                (std::numeric_limits<float>::max)();
            Vec3 towardPredecessor = {};

            for (int predecessorOffset = 0;
                 predecessorOffset < previousRow.positionCount;
                 ++predecessorOffset) {
                const Vec3& predecessor = positions[
                    previousRow.firstPositionIndex + predecessorOffset
                ];
                const float deltaX =
                    predecessor.x - positions[currentPositionIndex].x;
                const float deltaZ =
                    predecessor.z - positions[currentPositionIndex].z;
                const float distanceSquared = deltaX * deltaX + deltaZ * deltaZ;
                if (distanceSquared < closestDistanceSquared) {
                    closestDistanceSquared = distanceSquared;
                    towardPredecessor = { deltaX, 0.0f, deltaZ };
                }
            }

            if (!NormalizeXZ(towardPredecessor)) {
                failureReason = "could not determine a heading around a route turn";
                return false;
            }
            forwardDirections[currentPositionIndex] = towardPredecessor;
        }
    }

    return true;
}

} // namespace

bool CalculateThirtyCarGridPositions(
    int existingCarCount,
    int targetCarCount,
    std::array<Vec3, randomizerMaxCarCount>& outPositions,
    std::array<Vec3, randomizerMaxCarCount>& outForwardDirections
) {
    const char* failureReason = nullptr;

    if (!ValidateGridInputs(
            existingCarCount,
            targetCarCount,
            failureReason)) {
        return ReportGridFailure(failureReason);
    }

    std::array<Vec3, vanillaMaxCarCount> authoredPositions = {};
    std::array<Vec3, vanillaMaxCarCount> authoredForwardDirections = {};
    if (!LoadAuthoredStartGrid(
            authoredPositions,
            authoredForwardDirections,
            failureReason)) {
        return ReportGridFailure(failureReason);
    }

    NativeGridBasis basis;
    if (!EstablishGridBasis(
            authoredPositions,
            basis,
            failureReason)) {
        return ReportGridFailure(failureReason);
    }

    std::vector<GridRow> nativeRows;
    if (!GroupNativeRows(
            authoredPositions,
            authoredForwardDirections,
            basis,
            nativeRows,
            failureReason)) {
        return ReportGridFailure(failureReason);
    }

    NativeGridLayout nativeLayout;
    if (!BuildNativeCorridorDistances(
            nativeRows,
            nativeLayout,
            failureReason)) {
        return ReportGridFailure(failureReason);
    }

    float nativeGroundY = 0.0f;
    if (!FindGround(
            authoredPositions[0].x,
            authoredPositions[0].z,
            authoredPositions[0].y,
            nativeGroundY)) {
        failureReason = "could not find ground below the first native position";
        return ReportGridFailure(failureReason);
    }

    const float nativeHeightAboveGround =
        authoredPositions[0].y - nativeGroundY;
    if (!std::isfinite(nativeHeightAboveGround)) {
        failureReason = "native car height above ground is non-finite";
        return ReportGridFailure(failureReason);
    }

    std::vector<GeneratedGridRow> generatedRows;
    if (!GenerateGridRows(
            targetCarCount,
            nativeLayout,
            basis,
            nativeHeightAboveGround,
            outPositions,
            outForwardDirections,
            generatedRows,
            failureReason)) {
        return ReportGridFailure(failureReason);
    }

    if (!AssignGridHeadings(
            generatedRows,
            outPositions,
            outForwardDirections,
            failureReason)) {
        return ReportGridFailure(failureReason);
    }

    Logger::TimestampLogf(
        "[ThirtyCarGrid] CALCULATION SUCCEEDED: %d regridded positions generated",
        targetCarCount
    );
    return true;
}

} // namespace Randomizer
