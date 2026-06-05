#pragma once

#include <cstdint>
#include "RVGLFunctions.h"

namespace Randomizer {

using FnBuildMenu = void(*)(int slotIndex);
using FnHandleMenuAction = bool(*)(int slotIndex, uint32_t action);

extern FnBuildMenu Orig_BuildStartRaceMenu;
extern FnHandleMenuAction Orig_HandleStartRaceMenuAction;
extern FnRegisterMenuItemInActiveMenu Orig_RegisterMenuItemInActiveMenu;

// Frontend helpers exposed by the menu mod.
bool IncrementRandomizerCarCount(int panelIndex);
bool DecrementRandomizerCarCount(int panelIndex);
void PatchCarCountMenuDescriptor();
void SyncCarCountToVanillaSettings();

void Hook_BuildStartRaceMenu(int slotIndex);
bool Hook_HandleStartRaceMenuAction(int slotIndex, uint32_t action);
void Hook_RegisterMenuItemInActiveMenu(int slotIndex, int* descriptor);

} // namespace Randomizer
