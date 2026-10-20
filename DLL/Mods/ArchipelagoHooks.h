#pragma once

namespace Randomizer {

using FnMainGameLoop = int(*)(int argc, char** argv);

extern FnMainGameLoop Orig_MainGameLoop;

int Hook_MainGameLoop(int argc, char** argv);

}
