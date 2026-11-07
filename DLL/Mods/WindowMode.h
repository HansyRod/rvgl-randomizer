#pragma once

#include <cstdint>

namespace Randomizer {

// Stored alongside the other custom settings in each profile's mod INI.
struct WindowModeProfileSettings {
    int mode = -1; // -1 keeps the native mode when no preference has been saved.
};

WindowModeProfileSettings& GetWindowModeProfileSettings();
void ApplyWindowModeProfileSettings();

using FnApplyWindowMode = void(*)();
using FnBuildVideoSettingsMenu = void(*)(int slotIndex);
using FnDrawWindowModeMenuRow = void(*)(int slotIndex, uint32_t itemIndex, uint32_t visibleIndex);
using FnMinimizeGameWindow = void(*)(void* window);

extern FnApplyWindowMode Orig_ApplyWindowMode;
extern FnBuildVideoSettingsMenu Orig_BuildVideoSettingsMenu;
extern FnDrawWindowModeMenuRow Orig_DrawWindowModeMenuRow;
extern FnMinimizeGameWindow Orig_MinimizeGameWindow;

void ConfigureWindowModeLaunch(int argc, char** argv);
void Hook_ApplyWindowMode();
void Hook_BuildVideoSettingsMenu(int slotIndex);
void Hook_DrawWindowModeMenuRow(int slotIndex, uint32_t itemIndex, uint32_t visibleIndex);
void Hook_MinimizeGameWindow(void* window);

} // namespace Randomizer
