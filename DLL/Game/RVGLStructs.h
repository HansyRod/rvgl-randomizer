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

// ----------------------------------------------------------------------------
// CarInfo (272 bytes / 0x110)
//
// Static metadata for one car entry in the car pool.
// The pool starts at DAT_006fab50. Stride is sizeof(CarInfo) = 0x110.
// ----------------------------------------------------------------------------
struct CarInfo {
    char     internalName[20];    // +0x00  folder name (e.g. "rcbandit")
    char     displayName[64];     // +0x14  NAME keyword value
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
    int32_t  starRating;          // +0xEC  RATING keyword
    int32_t  obtainCondition;     // +0xF0  OBTAIN keyword
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

#pragma pack(pop)
