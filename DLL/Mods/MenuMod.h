#pragma once

#include <cstdint>
#include "RVGLFunctions.h"

namespace Randomizer {

using FnBuildMenu = void(*)(int slotIndex);
using FnHandleMenuAction = bool(*)(int slotIndex, uint32_t action);

extern FnBuildMenu Orig_BuildStartRaceMenu;
extern FnBuildMenu Orig_BuildOptionsMenu;
extern FnBuildMenu Orig_DrawPreRaceSummary;
extern FnBuildMenu Orig_BuildInRaceMenu;
extern FnHandleMenuAction Orig_HandleStartRaceMenuAction;
extern FnHandleMenuAction Orig_HandleOptionsMenuAction;
extern FnHandleMenuAction Orig_HandleInRaceMenuAction;

// Frontend helpers exposed by the menu mod.
bool IncrementRandomizerCarCount(int panelIndex);
bool DecrementRandomizerCarCount(int panelIndex);
void PatchCarCountMenuDescriptor();
void SyncCarCountToVanillaSettings();
void ApplyPendingInRaceCarSelection();

void Hook_BuildStartRaceMenu(int slotIndex);
void Hook_BuildOptionsMenu(int slotIndex);
void Hook_BuildInRaceMenu(int slotIndex);
bool Hook_HandleInRaceMenuAction(int slotIndex, uint32_t action);
void Hook_DrawPreRaceSummary(int slotIndex);
bool Hook_HandleOptionsMenuAction(int slotIndex, uint32_t action);
bool Hook_HandleStartRaceMenuAction(int slotIndex, uint32_t action);

} // namespace Randomizer
