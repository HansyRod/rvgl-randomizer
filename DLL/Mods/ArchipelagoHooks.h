#pragma once

namespace Randomizer {

using FnProcessSDLEvents = void(*)();
using FnMenuUpdateLoop = void(*)();
using FnInRaceGameLoop = void(*)();
using FnMainGameLoop = int(*)(int argc, char** argv);

extern FnProcessSDLEvents Orig_ProcessSDLEvents;
extern FnMenuUpdateLoop Orig_MenuUpdateLoop;
extern FnInRaceGameLoop Orig_InRaceGameLoop;
extern FnMainGameLoop Orig_MainGameLoop;

void Hook_ProcessSDLEvents();
void Hook_MenuUpdateLoop();
void Hook_InRaceGameLoop();
int Hook_MainGameLoop(int argc, char** argv);

}
