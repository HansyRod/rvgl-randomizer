#pragma once

namespace Randomizer {

using FnDrawProgressTable = void(*)(int slotIndex);

extern FnDrawProgressTable Orig_DrawProgressTable;

void Hook_DrawProgressTable(int slotIndex);
bool DecrementProgressTablePage(int panelIndex);
bool IncrementProgressTablePage(int panelIndex);
void ResetProgressTablePage();
void ClampProgressTablePage();
void InvalidateProgressTableCache();
void EnsureProgressLoaded(int trackIndex);

} // namespace Randomizer
