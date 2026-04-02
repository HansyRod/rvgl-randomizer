#pragma once
#include <cstdint>

// ============================================================================
// RVGLStructs.h
//
// Packed structs that mirror RVGL's in-memory data layouts.
// All offsets are verified against Ghidra decompiler output.
//
// Rules:
//   - Never add padding manually. Use #pragma pack(push, 1) / pop.
//   - Field names match those in the knowledge base documents.
//   - If a field's purpose is unknown, name it _pad_<hex offset> and note
//     its size. Do not guess.
// ============================================================================

#pragma pack(push, 1)

enum CarObtain : int32_t {
    NONE = -1,       // Can't be unlocked other than with cheats
    UNLOCKED = 0,    // Unlocked from the start
    CUP = 1,         // Unlocked by winning a championship cup
    TIME_TRIAL = 2,  // Unlocked by beating all challenge times on a tier
    PRACTICE = 3,    // Unlocked by finding all practice stars on a tier
    RACES = 4,       // Unlocked by winning all single races on a tier
};

enum CarRating : int32_t {
    ROOKIE = 0,
    AMATEUR = 1,
    ADVANCED = 2,
    SEMI_PRO = 3,
    PRO = 4,
    SUPER_PRO = 5
};

// ----------------------------------------------------------------------------
// CarInfo (272 bytes / 0x110)
//
// Static metadata for one car entry in the car pool.
// The pool starts at DAT_006fab50. Stride is sizeof(CarInfo) = 0x110.
// ----------------------------------------------------------------------------
struct CarInfo {
    char     displayName[20];     // +0x00  NAME keyword value
    char     internalName[64];    // +0x14  folder name (e.g. "rcbandit")
    char     tpageFilename[64];   // +0x54  TPAGE keyword
    char     tcarboxFilename[64]; // +0x94  TCARBOX keyword
    uint8_t  _pad_D4[4];          // +0xD4
    char*    pSkinNames;          // +0xD8  heap array of skin name strings (char[12] each)
    int32_t  skinCount;           // +0xE0  number of skin entries (min 1)
    bool     bestTimeEnabled;     // +0xE4  BESTTIME keyword
    bool     selectableByPlayer;  // +0xE5  SELECTABLE keyword
    bool     selectableByCPU;     // +0xE6  CPUSELECTABLE keyword
    bool     statisticsEnabled;   // +0xE7  STATISTICS keyword
    int32_t  carClass;            // +0xE8  CLASS keyword (0–4)
    CarRating rating;             // +0xEC  RATING keyword
    CarObtain obtainCondition;    // +0xF0  OBTAIN keyword
    float    topSpeedStat;        // +0xF4  TOPEND keyword
    float    accelerationStat;    // +0xF8  ACC keyword
    float    weightValue;         // +0xFC  WEIGHT keyword
    int32_t  transmissionType;    // +0x100 TRANS keyword
    uint32_t storedChecksum;      // +0x104 CRC32 from CheckFileIntegrity
    uint8_t  statusFlags;         // +0x108 0 = valid, 1 = bad checksum
    bool     isInvalid;           // +0x109 1 = car failed to load
    uint8_t  _pad_10A[6];         // +0x10A
};
static_assert(sizeof(CarInfo) == 0x110, "CarInfo size mismatch");

// Availability flags for TrackInfo::trackAvailFlags
// Computed fresh each session by Track_UpdateAvailabilityFlags.
// Controls which race modes the player can select for a given track.
enum TrackAvailFlags : uint32_t {
    TRACKAVAIL_EXISTS   = 0x01,  // Track world file exists on disk
    TRACKAVAIL_NORMAL   = 0x02,  // Normal racing available
    TRACKAVAIL_REVERSE  = 0x04,  // Reverse mode available
    TRACKAVAIL_TT       = 0x08,  // Time Trial available
    TRACKAVAIL_MIRROR   = 0x10,  // Mirror mode available
};

// Progress flags for TrackInfo::trackProgressFlags
// Persisted in the player's save data. Records what the player has
// achieved on this track, used to gate mode unlocks in Track_ApplyCustomUnlock.
enum TrackProgressFlags : uint32_t {
    TRACKPROGRESS_NORMAL_CHALLENGE_BEATEN   = 0x01,  // Normal challenge time beaten
    TRACKPROGRESS_REVERSE_CHALLENGE_BEATEN  = 0x02,  // Reverse challenge time beaten
    TRACKPROGRESS_MIRROR_CHALLENGE_BEATEN   = 0x04,  // Mirror/third challenge beaten
    TRACKPROGRESS_PRACTICE_STAR             = 0x08,  // Practice star earned
    TRACKPROGRESS_RACE_WON                  = 0x10,  // Race won
    TRACKPROGRESS_COMPLETED                 = 0x20,  // Track completed (bronze)
    TRACKPROGRESS_PROGRESS_LOADED           = 0x80000000, // Progress file already loaded this session
};

struct TrackInfo {
    char               folderName[16];          // +0x00
    char               displayName[64];          // +0x10
    float              trackLengthNormal;        // +0x50
    float              trackLengthReverse;       // +0x54
    TrackProgressFlags trackProgressFlags;       // +0x58
    TrackAvailFlags    trackAvailFlags;          // +0x5C
    uint32_t           customUnlockType;         // +0x60
    int32_t            difficultyRating;         // +0x64
    int32_t            gameType;                 // +0x68
    int32_t            challengeTime;            // +0x6C
    int32_t            challengeReverseTime;     // +0x70
    uint32_t           onlineVerifyChecksum;     // +0x74  ← was unknown3
};
static_assert(sizeof(TrackInfo) == 0x78, "TrackInfo size mismatch");

// Reconstructed UV structure starting at offset 0x38 in the carbox object.
struct CarboxUV {
    uint16_t unknown;
    uint16_t textureSlot; // 0x8F to 0x93 for vanilla grids, 0x8E for custom
    float u0, v0;         // Bottom Right
    float u1, v1;         // Top Right
    float u2, v2;         // Top Left
    float u3, v3;         // Bottom Left
};

// Reconstructed frontend carbox structure
struct CarboxData {
    int carIndex;          // +0x00: Index in the CarArrayPtr
    int unknown1;          // +0x04
    int unknown2;          // +0x08
    int unknown3;          // +0x0C
    char padding[40];      // ... padding ...
    CarboxUV* uvData;      // +0x38: (param_1 + 0xE in 32-bit integer arithmetic)
};

// Stride for wheel sub-struct: 0x54 = 84 bytes (21 × int32)
struct WheelPhysics {           // 0x54 bytes
    int32_t  modelNum;            // +0x00
    float  offset1[3];          // +0x04
    float  offset2[3];          // +0x10
    float  radius;              // +0x1c
    float  mass;                // +0x20
    float  gravity;             // +0x24
    float  grip;                // +0x28
    float  staticFriction;      // +0x2c
    float  kineticFriction;     // +0x30
    float  axleFriction;        // +0x34
    float  steerRatio;          // +0x38
    float  engineRatio;         // +0x3c
    float  maxPos;              // +0x40
    float  skidWidth;           // +0x44
    float  toeIn;               // +0x48
    float  camber;              // +0x4c
    bool   isPresent;           // +0x50
    bool   isTurnable;          // +0x51
    bool   isPowered;           // +0x52
    char   pad;                 // +0x53
};

struct SpringPhysics {          // 0x20 bytes (stride)
    int32_t  modelNum;            // +0x00  (stored as float, written as %d)
    float  offset[3];           // +0x04
    float  length;              // +0x10
    float  stiffness;           // +0x14
    float  damping;             // +0x18
    float  restitution;         // +0x1c
};

struct AxleOrPinPhysics {       // 0x14 bytes (stride)
    int32_t  modelNum;            // +0x00
    float  offset[3];           // +0x04
    float  length;              // +0x10
};

struct CarPhysicsData {         // ~0xAAC bytes
    char      internalName[20]; // +0x000
    char 	  modelFilenames[19][64]; // +0x014 (model filename table: 19 entries × ~52 bytes)
    char      tpageFilename[64];// +0x4d4
    char      tcarboxFilename[64];// +0x514
    char      shadowFilename[64];// +0x554
    char      collFilename[64]; // +0x594
    char      sfxEngineName[64];// +0x5d4
    char      sfxServoName[64]; // +0x614
    char      sfxHonkName[64];  // +0x654
    float     shadowTable[5];   // +0x694  left,right,front,back,height
	float     shadowTexParams[5];    // +0x6A8  unknown, related to shadow config
    int32_t     shadowIndex;      // +0x6bc
    uint32_t    envRGB;           // +0x6c0  packed 0x00RRGGBB
    float     steerRate;        // +0x6c4
    float     steerMod;         // +0x6c8
    float     engineRate;       // +0x6cc
    float     topSpeed;         // +0x6d0  (mph × 89.445)
    float     maxRevs;          // +0x6d4  (same scale)
    float     downForceMod;     // +0x6d8
    float     centerOfMass[3];  // +0x6dc
    float     weaponOffset1[3]; // +0x6e8
    float     weaponOffset2[3]; // +0x6f4
    bool      bestTime;         // +0x700
    bool      selectable;       // +0x701
    bool      cpuSelectable;    // +0x702
    bool      statistics;       // +0x703
    bool      flippable;        // +0x704
    bool      flying;           // +0x705
    char      clothFx;          // +0x706
    bool      camUseDefault;    // +0x707
    float     camHoodOffset[3]; // +0x708
    float     camHoodLook;      // +0x714
    float     camRearOffset[3]; // +0x718
    float     camRearLook;      // +0x724
    char      camFixedOffset;   // +0x728
    char      camFixedLook;     // +0x729
    char      pad_72a[2];       // +0x72a
    int32_t     bodyModelNum;     // +0x72c
    float     bodyOffset[3];    // +0x730
    float     bodyMass;         // +0x73c
    float     bodyInertia[9];   // +0x740  3×3 matrix
    float     bodyGravity;      // +0x764  (unused)
    float     bodyHardness;     // +0x768
    float     bodyResistance;   // +0x76c
    float     bodyAngRes;       // +0x770
    float     bodyResMod;       // +0x774
    float     bodyGrip;         // +0x778
    float     bodyStaticFric;   // +0x77c
    float     bodyKineticFric;  // +0x780
    WheelPhysics  wheel[4];     // +0x784  4 × 0x54 = 0x150 bytes
    SpringPhysics spring[4];    // +0x8d4  4 × 0x20 = 0x80 bytes
    AxleOrPinPhysics axle[4];   // +0x954  4 × 0x14 = 0x50 bytes
    AxleOrPinPhysics pin[4];    // +0x9a4  4 × 0x14 = 0x50 bytes
    int32_t     spinnerModelNum;  // +0x9f4  (stored as float, written as %d)
    float     spinnerOffset[3]; // +0x9f8
    float     spinnerAxis[3];   // +0xa04
    float     spinnerTrans[3];  // +0xa10
    float     spinnerAngVel;    // +0xa1c
    float     spinnerTransVel;  // +0xa20
    int32_t     spinnerType;      // +0xa24  (int semantics; type 1/2/4/6)
    int32_t     aerialSecModel;   // +0xa28  (int semantics)
    int32_t     aerialTopModel;   // +0xa2c  (int semantics)
    float     aerialOffset[3];  // +0xa30
    float     aerialDirection[3];// +0xa3c
    float     aerialLength;     // +0xa48
    float     aerialStiffness;  // +0xa4c
    float     aerialDamping;    // +0xa50
    float     aiUnderThresh;    // +0xa54
    float     aiUnderRange;     // +0xa58
    float     aiUnderFront;     // +0xa5c
    float     aiUnderRear;      // +0xa60
    float     aiUnderMax;       // +0xa64
    float     aiOverThresh;     // +0xa68
    float     aiOverRange;      // +0xa6c
    float     aiOverMax;        // +0xa70
    float     aiOverAccThresh;  // +0xa74
    float     aiOverAccRange;   // +0xa78
    float     aiPickupBias;     // +0xa7c  (int semantics)
    float     aiBlockBias;      // +0xa80  (int semantics)
    float     aiOvertakeBias;   // +0xa84  (int semantics)
    float     aiSuspension;     // +0xa88  (int semantics)
    float     aiAggression;     // +0xa8c  (int semantics)
    int32_t     carClass;         // +0xa90
    int32_t     starRating;       // +0xa94
    int32_t     obtainCondition;  // +0xa98
    float     topEndStat;       // +0xa9c
    float     accStat;          // +0xaa0
    float     weightStat;       // +0xaa4
    int32_t     transmission;     // +0xaa8
};  // Total: ~0xAAC bytes

#pragma pack(pop)
