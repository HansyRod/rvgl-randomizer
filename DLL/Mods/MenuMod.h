#pragma once

namespace Randomizer {

// Frontend helpers exposed by the menu mod.
bool IncrementRandomizerCarCount(int panelIndex);
bool DecrementRandomizerCarCount(int panelIndex);
void PatchCarCountMenuDescriptor();
void SyncCarCountToVanillaSettings();

} // namespace Randomizer
