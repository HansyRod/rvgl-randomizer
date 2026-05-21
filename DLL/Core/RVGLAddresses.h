#pragma once

#include <cstdint>

#if defined(__linux__)
#include "Addresses_Linux64.h"
#else
#define RVGL_RANDOMIZER_USE_PLATFORM_ADDRESS_RESOLVER
#include "Addresses.h"
#undef RVGL_RANDOMIZER_USE_PLATFORM_ADDRESS_RESOLVER
#endif

namespace RVGL {

uintptr_t ModuleBase();
uintptr_t AbsFromRva(uint32_t rva);
uint8_t* GetTrackProgressCache();

} // namespace RVGL

inline uintptr_t AbsFromRva(uint32_t rva) {
    return RVGL::AbsFromRva(rva);
}
