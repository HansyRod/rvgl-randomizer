#pragma once

#include "MenuMod.h"
#include <string>

namespace Randomizer {

extern FnBuildMenu Orig_BuildMainMenu;
extern FnHandleMenuAction Orig_HandleMainMenuAction;
void Hook_BuildMainMenu(int slotIndex);
bool Hook_HandleMainMenuAction(int slotIndex, uint32_t action);
void UpdateArchipelagoMenu();
void ResetArchipelagoMenuProfile(const std::string& server);

} // namespace Randomizer
