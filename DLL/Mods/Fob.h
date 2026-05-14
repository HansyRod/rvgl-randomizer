#pragma once

namespace Randomizer {

// Hooked RVGL FOB loader.
using FnLoadObjectsFromFob = void(*)(char* fobFilePath);

// MinHook writes the trampoline address into this during InstallAll().
extern FnLoadObjectsFromFob Orig_LoadObjectsFromFob;

void Hook_LoadObjectsFromFob(char* fobFilePath);

} // namespace Randomizer
