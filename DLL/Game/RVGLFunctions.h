#pragma once

#include "RVGLRaceStructs.h"

namespace Randomizer {

using FnCreateCarEntity = CarEntityRuntime*(*)(int initialState, int controllerType, int carModelId, uint64_t skinIdAndFlags, float* spawnPosition, float* spawnOrientation);
using FnComputeSpawnOrientation = void(*)(int startSlot, float* outPosition, float* outOrientation);
using FnSetCarTransform = void(*)(CarTransformRuntime* transform, float* position, float* orientation);
using FnDrawNumericMenuValue = void(*)(int panelIndex, int itemIndex, void* unused, void* renderContext);
using FnDecrementNumericMenuValue = bool(*)(int panelIndex);
using FnIncrementNumericMenuValue = bool(*)(int panelIndex);
using FnCreateObjectFromFob = void*(*)(float* position, float* rotationMatrix, unsigned int objectId, int* subinfos);

extern FnCreateCarEntity RVGL_CreateCarEntity;
extern FnComputeSpawnOrientation RVGL_ComputeSpawnOrientation;
extern FnSetCarTransform RVGL_SetCarTransform;
extern FnDrawNumericMenuValue RVGL_DrawNumericMenuValue;
extern FnDecrementNumericMenuValue RVGL_DecrementNumericMenuValue;
extern FnIncrementNumericMenuValue RVGL_IncrementNumericMenuValue;
extern FnCreateObjectFromFob RVGL_CreateObjectFromFob;

} // namespace Randomizer
