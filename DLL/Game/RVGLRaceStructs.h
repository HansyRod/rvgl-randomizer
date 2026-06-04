#pragma once
#include <cstddef>
#include <cstdint>

// ============================================================================
// RVGLRaceStructs.h
//
// Shared RVGL runtime layouts used by race/session-related mods.
// Keep these in Game/ so they are declared once and reused everywhere.
// ============================================================================

#pragma pack(push, 1)

struct Vec3 {
    float x;
    float y;
    float z;
};

struct PhysicsBodyRuntime {
    uint8_t _pad_00[20];
    Vec3 position;                  // +0x14
    Vec3 velocity;                  // +0x20
    uint8_t _pad_2C[40];
    float orientationMatrix[9];     // +0x54
};

struct CarTransformRuntime {
    int32_t modelId;                 // +0x000
    uint8_t _pad_004[124];
    PhysicsBodyRuntime* physicsBody; // +0x080
    uint8_t _pad_088[3416];
    Vec3 cachedPosition;             // +0xDE0
    uint8_t _pad_DEC[204];
};

struct CarEntityRuntime {
    int32_t nCarArrayIndex;          // +0x0000
    int32_t carState;                // +0x0004
    CarEntityRuntime* pPrev;         // +0x0008
    CarEntityRuntime* pNext;         // +0x0010
    uint8_t _pad_018[48];
    CarTransformRuntime transform;   // +0x0048
    uint8_t _pad_0F00[23232];
    int32_t racePositionIndex;       // +0x69C0, zero-based
    uint8_t _pad_69C4[132];
    int32_t finishTimeMs;            // +0x6A48
    int32_t finishPosition;          // +0x6A4C, zero-based
    uint8_t _pad_6A50[32];
    char driverName[56];             // +0x6A70
};

struct RaceParticipantRuntime {
    int32_t carType;                 // +0x00
    int32_t startSlot;               // +0x04
    int32_t modelId;                 // +0x08
    int32_t skinId;                  // +0x0C
    int32_t reserved10;              // +0x10
    int32_t isLocal;                 // +0x14
    int32_t networkId;               // +0x18
    int32_t hasCheated;              // +0x1C
    char carName[20];                // +0x20
    char skinName[12];               // +0x34
    char playerName[16];             // +0x40
};

#pragma pack(pop)

static_assert(sizeof(void*) == 8, "RVGL runtime layouts here assume a 64-bit process.");
static_assert(offsetof(PhysicsBodyRuntime, position) == 0x14, "PhysicsBodyRuntime::position offset mismatch.");
static_assert(offsetof(PhysicsBodyRuntime, orientationMatrix) == 0x54, "PhysicsBodyRuntime::orientationMatrix offset mismatch.");
static_assert(offsetof(CarTransformRuntime, physicsBody) == 0x80, "CarTransformRuntime::physicsBody offset mismatch.");
static_assert(offsetof(CarTransformRuntime, cachedPosition) == 0xDE0, "CarTransformRuntime::cachedPosition offset mismatch.");
static_assert(sizeof(CarTransformRuntime) == 0xEB8, "CarTransformRuntime size mismatch.");
static_assert(offsetof(CarEntityRuntime, transform) == 0x48, "CarEntityRuntime::transform offset mismatch.");
static_assert(offsetof(CarEntityRuntime, racePositionIndex) == 0x69C0, "CarEntityRuntime::racePositionIndex offset mismatch.");
static_assert(offsetof(CarEntityRuntime, finishTimeMs) == 0x6A48, "CarEntityRuntime::finishTimeMs offset mismatch.");
static_assert(offsetof(CarEntityRuntime, finishPosition) == 0x6A4C, "CarEntityRuntime::finishPosition offset mismatch.");
static_assert(offsetof(CarEntityRuntime, driverName) == 0x6A70, "CarEntityRuntime::driverName offset mismatch.");
static_assert(sizeof(CarEntityRuntime) == 0x6AA8, "CarEntityRuntime size mismatch.");
static_assert(sizeof(RaceParticipantRuntime) == 0x50, "RaceParticipantRuntime size mismatch.");
static_assert(offsetof(RaceParticipantRuntime, modelId) == 0x08, "RaceParticipantRuntime::modelId offset mismatch.");
static_assert(offsetof(RaceParticipantRuntime, playerName) == 0x40, "RaceParticipantRuntime::playerName offset mismatch.");
