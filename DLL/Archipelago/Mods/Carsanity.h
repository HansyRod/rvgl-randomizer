#pragma once

namespace Randomizer {

using FnCarSelectionHandler = void(*)(int slotIndex, int scrollY);
extern FnCarSelectionHandler Orig_CarSelectionHandler;

void ResetCarsanityRaceState();
void UpdateCarsanityRaceProgress();
void Hook_CarSelectionHandler(int slotIndex, int scrollY);

} // namespace Randomizer
