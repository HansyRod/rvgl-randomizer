#pragma once

namespace Randomizer {

using FnProcessSDLEvents = void(*)();
using FnMenuUpdateLoop = void(*)();
using FnInRaceGameLoop = void(*)();
using FnMainGameLoop = int(*)(int argc, char** argv);
using FnUpdateRaceState = void(*)();
using FnScreenshotAndFrameEnd = void(*)();

extern FnProcessSDLEvents Orig_ProcessSDLEvents;
extern FnMenuUpdateLoop Orig_MenuUpdateLoop;
extern FnInRaceGameLoop Orig_InRaceGameLoop;
extern FnMainGameLoop Orig_MainGameLoop;
extern FnUpdateRaceState Orig_UpdateRaceState;
extern FnScreenshotAndFrameEnd Orig_ScreenshotAndFrameEnd;

void Hook_ProcessSDLEvents();
void Hook_MenuUpdateLoop();
void Hook_InRaceGameLoop();
int Hook_MainGameLoop(int argc, char** argv);
void Hook_UpdateRaceState();
void Hook_ScreenshotAndFrameEnd();

}
