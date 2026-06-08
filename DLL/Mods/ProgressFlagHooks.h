#pragma once

#include "RVGLStructs.h"
#include <cstdint>
#include <cstdio>

namespace Randomizer {

    using FnUpdateTimeTrialLeaderboards = void(*)(int* carRaceData);
    using FnPickup_CollectProgressObject = void(*)(void* pickup);
    using FnEngine_UpdateRaceProgress = void(*)();
    using FnCup_OnStageFinished = void(*)();

    extern FnUpdateTimeTrialLeaderboards Orig_UpdateTimeTrialLeaderboards;
    extern FnPickup_CollectProgressObject Orig_Pickup_CollectProgressObject;
    extern FnEngine_UpdateRaceProgress Orig_Engine_UpdateRaceProgress;
    extern FnCup_OnStageFinished Orig_Cup_OnStageFinished;

    void Hook_UpdateTimeTrialLeaderboards(int* carRaceData);
    void Hook_Pickup_CollectProgressObject(void* pickup);
    void Hook_Engine_UpdateRaceProgress();
    void Hook_Cup_OnStageFinished();

} // namespace Randomizer
