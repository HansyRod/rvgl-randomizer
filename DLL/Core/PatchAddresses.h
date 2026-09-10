#pragma once

#include <cstdint>

// ============================================================================
// PatchAddresses.h
//
// RVAs for individual native instruction patches. These are kept separate
// from Addresses.h because they identify patch locations rather than callable
// functions or runtime data used by the mod.
// ============================================================================

// Instruction inside LoadCustomCarPool that supplies strlen(candidateName)
// as the maximum length for the vanilla/custom duplicate-name comparison.
constexpr uint32_t RVA_CUSTOM_CAR_NAME_COMPARE_LENGTH = 0x0003fbd3;
