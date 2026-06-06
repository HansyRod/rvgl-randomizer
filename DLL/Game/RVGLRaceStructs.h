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

struct PhysicsEntityRuntime {
    uint8_t _pad_000[0x308];
    int32_t collisionMode;           // +0x308
    uint8_t _pad_30C[0x34];
    void* primaryUpdate;             // +0x340
    void* secondaryUpdate;           // +0x348
    void* stateUpdate;               // +0x350
    void* collisionUpdate;           // +0x358
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
    uint8_t _pad_018[40];
    PhysicsEntityRuntime* physicsEntity; // +0x0040
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

struct RaceSettingsRuntime {
    int32_t mode;                     // +0x00
    int32_t selectedCupIndex;         // +0x04
    uint8_t _pad_08[32];
    char playerName[16];              // +0x28
    uint8_t _pad_38[48];
    int32_t playerModelId;            // +0x68
    uint8_t _pad_6C[12];
    int32_t playerSkinId;             // +0x78
    uint8_t _pad_7C[34];
    uint8_t pickupsEnabled;           // +0x9E
};

struct GameModeRuntime {
    int32_t mode;                     // +0x00
    uint8_t _pad_04[4];
    int32_t trackId;                  // +0x08
    uint8_t _pad_0C[8];
    int32_t countdown;                // +0x14
    int32_t laps;                     // +0x18
    uint8_t _pad_1C[20];
    uint8_t reverse;                  // +0x30
    uint8_t mirror;                   // +0x31
    uint8_t pickupsEnabled;           // +0x32
};

struct PlayerRaceInfoRuntime {
    uint8_t _pad_00[4];
    int32_t participantCount;         // +0x04
    uint8_t _pad_08[4];
    int32_t mode;                     // +0x0C
    int32_t laps;                     // +0x10
    int32_t mirror;                   // +0x14
    int32_t reverse;                  // +0x18
    uint8_t _pad_1C[4];
    int32_t pickupsEnabled;           // +0x20
    uint8_t _pad_24[4];
    int32_t countdown;                // +0x28
    uint8_t _pad_2C[8];
    int32_t unknown34;                // +0x34
    uint8_t _pad_38[8];
    char trackFolder[16];             // +0x40
};

struct UiViewportRuntime {
    uint8_t _pad_00[16];
    float centerX;                    // +0x10
    float centerY;                    // +0x14
};

#pragma pack(pop)

static_assert(sizeof(void*) == 8, "RVGL runtime layouts here assume a 64-bit process.");
static_assert(offsetof(PhysicsEntityRuntime, collisionMode) == 0x308, "PhysicsEntityRuntime::collisionMode offset mismatch.");
static_assert(offsetof(PhysicsEntityRuntime, primaryUpdate) == 0x340, "PhysicsEntityRuntime::primaryUpdate offset mismatch.");
static_assert(offsetof(PhysicsEntityRuntime, stateUpdate) == 0x350, "PhysicsEntityRuntime::stateUpdate offset mismatch.");
static_assert(offsetof(PhysicsEntityRuntime, collisionUpdate) == 0x358, "PhysicsEntityRuntime::collisionUpdate offset mismatch.");
static_assert(offsetof(PhysicsBodyRuntime, position) == 0x14, "PhysicsBodyRuntime::position offset mismatch.");
static_assert(offsetof(PhysicsBodyRuntime, orientationMatrix) == 0x54, "PhysicsBodyRuntime::orientationMatrix offset mismatch.");
static_assert(offsetof(CarTransformRuntime, physicsBody) == 0x80, "CarTransformRuntime::physicsBody offset mismatch.");
static_assert(offsetof(CarTransformRuntime, cachedPosition) == 0xDE0, "CarTransformRuntime::cachedPosition offset mismatch.");
static_assert(sizeof(CarTransformRuntime) == 0xEB8, "CarTransformRuntime size mismatch.");
static_assert(offsetof(CarEntityRuntime, physicsEntity) == 0x40, "CarEntityRuntime::physicsEntity offset mismatch.");
static_assert(offsetof(CarEntityRuntime, transform) == 0x48, "CarEntityRuntime::transform offset mismatch.");
static_assert(offsetof(CarEntityRuntime, racePositionIndex) == 0x69C0, "CarEntityRuntime::racePositionIndex offset mismatch.");
static_assert(offsetof(CarEntityRuntime, finishTimeMs) == 0x6A48, "CarEntityRuntime::finishTimeMs offset mismatch.");
static_assert(offsetof(CarEntityRuntime, finishPosition) == 0x6A4C, "CarEntityRuntime::finishPosition offset mismatch.");
static_assert(offsetof(CarEntityRuntime, driverName) == 0x6A70, "CarEntityRuntime::driverName offset mismatch.");
static_assert(sizeof(CarEntityRuntime) == 0x6AA8, "CarEntityRuntime size mismatch.");
static_assert(sizeof(RaceParticipantRuntime) == 0x50, "RaceParticipantRuntime size mismatch.");
static_assert(offsetof(RaceParticipantRuntime, modelId) == 0x08, "RaceParticipantRuntime::modelId offset mismatch.");
static_assert(offsetof(RaceParticipantRuntime, playerName) == 0x40, "RaceParticipantRuntime::playerName offset mismatch.");
static_assert(offsetof(RaceSettingsRuntime, mode) == 0x00, "RaceSettingsRuntime::mode offset mismatch.");
static_assert(offsetof(RaceSettingsRuntime, selectedCupIndex) == 0x04, "RaceSettingsRuntime::selectedCupIndex offset mismatch.");
static_assert(offsetof(RaceSettingsRuntime, playerName) == 0x28, "RaceSettingsRuntime::playerName offset mismatch.");
static_assert(offsetof(RaceSettingsRuntime, playerModelId) == 0x68, "RaceSettingsRuntime::playerModelId offset mismatch.");
static_assert(offsetof(RaceSettingsRuntime, playerSkinId) == 0x78, "RaceSettingsRuntime::playerSkinId offset mismatch.");
static_assert(offsetof(RaceSettingsRuntime, pickupsEnabled) == 0x9E, "RaceSettingsRuntime::pickupsEnabled offset mismatch.");
static_assert(offsetof(GameModeRuntime, mode) == 0x00, "GameModeRuntime::mode offset mismatch.");
static_assert(offsetof(GameModeRuntime, trackId) == 0x08, "GameModeRuntime::trackId offset mismatch.");
static_assert(offsetof(GameModeRuntime, countdown) == 0x14, "GameModeRuntime::countdown offset mismatch.");
static_assert(offsetof(GameModeRuntime, laps) == 0x18, "GameModeRuntime::laps offset mismatch.");
static_assert(offsetof(GameModeRuntime, reverse) == 0x30, "GameModeRuntime::reverse offset mismatch.");
static_assert(offsetof(GameModeRuntime, mirror) == 0x31, "GameModeRuntime::mirror offset mismatch.");
static_assert(offsetof(GameModeRuntime, pickupsEnabled) == 0x32, "GameModeRuntime::pickupsEnabled offset mismatch.");
static_assert(offsetof(PlayerRaceInfoRuntime, participantCount) == 0x04, "PlayerRaceInfoRuntime::participantCount offset mismatch.");
static_assert(offsetof(PlayerRaceInfoRuntime, mode) == 0x0C, "PlayerRaceInfoRuntime::mode offset mismatch.");
static_assert(offsetof(PlayerRaceInfoRuntime, laps) == 0x10, "PlayerRaceInfoRuntime::laps offset mismatch.");
static_assert(offsetof(PlayerRaceInfoRuntime, mirror) == 0x14, "PlayerRaceInfoRuntime::mirror offset mismatch.");
static_assert(offsetof(PlayerRaceInfoRuntime, reverse) == 0x18, "PlayerRaceInfoRuntime::reverse offset mismatch.");
static_assert(offsetof(PlayerRaceInfoRuntime, pickupsEnabled) == 0x20, "PlayerRaceInfoRuntime::pickupsEnabled offset mismatch.");
static_assert(offsetof(PlayerRaceInfoRuntime, countdown) == 0x28, "PlayerRaceInfoRuntime::countdown offset mismatch.");
static_assert(offsetof(PlayerRaceInfoRuntime, unknown34) == 0x34, "PlayerRaceInfoRuntime::unknown34 offset mismatch.");
static_assert(offsetof(PlayerRaceInfoRuntime, trackFolder) == 0x40, "PlayerRaceInfoRuntime::trackFolder offset mismatch.");
static_assert(offsetof(UiViewportRuntime, centerX) == 0x10, "UiViewportRuntime::centerX offset mismatch.");
static_assert(offsetof(UiViewportRuntime, centerY) == 0x14, "UiViewportRuntime::centerY offset mismatch.");
