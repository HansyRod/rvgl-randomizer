#pragma once

#include <cstdint>
#include "RVGLFunctions.h"

namespace Randomizer {

using FnBuildMenu = void(*)(int slotIndex);
using FnHandleMenuAction = bool(*)(int slotIndex, uint32_t action);

extern FnBuildMenu Orig_BuildStartRaceMenu;
extern FnBuildMenu Orig_BuildOptionsMenu;
extern FnHandleMenuAction Orig_HandleStartRaceMenuAction;

// Frontend helpers exposed by the menu mod.
bool IncrementRandomizerCarCount(int panelIndex);
bool DecrementRandomizerCarCount(int panelIndex);
void PatchCarCountMenuDescriptor();
void SyncCarCountToVanillaSettings();

void Hook_BuildStartRaceMenu(int slotIndex);
void Hook_BuildOptionsMenu(int slotIndex);
bool Hook_HandleStartRaceMenuAction(int slotIndex, uint32_t action);

} // namespace Randomizer
