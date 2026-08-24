#pragma once

#include "RVGLRaceStructs.h"
#include <cstdint>
#include <cstdio>

namespace Randomizer {

using FnCreateCarEntity = CarEntityRuntime*(*)(int initialState, int controllerType, int carModelId, uint64_t skinIdAndFlags, float* spawnPosition, float* spawnOrientation);
using FnComputeSpawnOrientation = void(*)(int startSlot, float* outPosition, float* outOrientation);
using FnSetCarTransform = void(*)(CarTransformRuntime* transform, float* position, float* orientation);
using FnGetCollisionGridCellIndex = int(*)(Vec3* worldPoint);
// The native function communicates its result through hitFraction and hitPlane.
// Its return register is always non-zero, so it must not be treated as a hit test.
using FnTraceSegmentAgainstCollisionGrid = void(*)(Vec3* startPoint, Vec3* endPoint, float* hitFraction, CollisionPlane** hitPlane);
using FnFindTrackZoneForCarBruteForce = bool(*)(TrackZoneQueryContext* carTrackState);
using FnDrawNumericMenuValue = void(*)(int panelIndex, int itemIndex);
using FnDecrementNumericMenuValue = bool(*)(int panelIndex);
using FnIncrementNumericMenuValue = bool(*)(int panelIndex);
using FnCreateObjectFromFob = void*(*)(float* position, float* rotationMatrix, unsigned int objectId, int* subinfos);
using FnVfsResolvePath = char*(*)(char* filename, char* outBuffer, char resolveFlags, bool isWriteMode, void* context);
using FnTrackFileExists = char(*)(int trackIndex);
using FnTrackReversedDirExists = bool(*)(int trackIndex);
using FnTrackLoadProgressFromFile = void(*)(int trackIndex);
using FnTrackIsAvailableForFrontend = int(*)(int trackIndex, bool ignoreUnlocks);
using FnDrawUIText = void(*)(float x, float y, float width, float height, uint32_t rgba, char* text, float maxWidth, uint8_t flags);
using FnDrawSprite2D = void(*)(float x, float y, float width, float height, float u, float v, float uWidth, float vHeight, uint32_t rgba, int textureSlot, int mode);
using FnUIDrawRoundedRect = void(*)(float x, float y, float width, float height, int panelId, int style, uint32_t rgba, int alpha, int flags);
using FnSetupGLRenderState = void(*)();
using FnFlushDeferredUIBatches = void(*)();
using FnUTF8GetVisibleCharCount = int64_t(*)(char* text);
using FnResetCurrentTrackSelectionState = void(*)();
using FnRaceTeardownAndSave = void(*)();
using FnLevelDestroyAndFree = void(*)();
using FnLoadNextRaceFromPlayerRaceInfo = void(*)();
using FnRegisterMenuItemInActiveMenu = void(*)(int slotIndex, int* descriptor);
using FnRegisterFinishTime = void(*)(int* car, uint32_t finishTimeMs, uint8_t dnfFlag);
using FnSetCarBehaviourState = void(*)(CarEntityRuntime* car, int state);
using FnSetPostRacePopup = void(*)(char* prefixText, char* timeText, uint32_t prefixColor, uint32_t timeColor, int priority, float duration);

extern FnCreateCarEntity RVGL_CreateCarEntity;
extern FnComputeSpawnOrientation RVGL_ComputeSpawnOrientation;
extern FnSetCarTransform RVGL_SetCarTransform;
extern FnGetCollisionGridCellIndex RVGL_GetCollisionGridCellIndex;
extern FnTraceSegmentAgainstCollisionGrid RVGL_TraceSegmentAgainstCollisionGrid;
extern FnFindTrackZoneForCarBruteForce RVGL_FindTrackZoneForCarBruteForce;
extern FnDrawNumericMenuValue RVGL_DrawNumericMenuValue;
extern FnDecrementNumericMenuValue RVGL_DecrementNumericMenuValue;
extern FnIncrementNumericMenuValue RVGL_IncrementNumericMenuValue;
extern FnCreateObjectFromFob RVGL_CreateObjectFromFob;
extern FnVfsResolvePath RVGL_VfsResolvePath;
extern FnTrackFileExists RVGL_TrackFileExists;
extern FnTrackReversedDirExists RVGL_TrackReversedDirExists;
extern FnTrackLoadProgressFromFile RVGL_TrackLoadProgressFromFile;
extern FnTrackIsAvailableForFrontend RVGL_TrackIsAvailableForFrontend;
extern FnDrawUIText RVGL_DrawUIText;
extern FnDrawSprite2D RVGL_DrawSprite2D;
extern FnUIDrawRoundedRect RVGL_UIDrawRoundedRect;
extern FnSetupGLRenderState RVGL_SetupGLRenderState;
extern FnFlushDeferredUIBatches RVGL_FlushDeferredUIBatches;
extern FnUTF8GetVisibleCharCount RVGL_UTF8GetVisibleCharCount;
extern FnResetCurrentTrackSelectionState RVGL_ResetCurrentTrackSelectionState;
extern FnRaceTeardownAndSave RVGL_RaceTeardownAndSave;
extern FnLevelDestroyAndFree RVGL_LevelDestroyAndFree;
extern FnLoadNextRaceFromPlayerRaceInfo RVGL_LoadNextRaceFromPlayerRaceInfo;
extern FnRegisterMenuItemInActiveMenu RVGL_RegisterMenuItemInActiveMenu;
extern FnRegisterFinishTime RVGL_RegisterFinishTime;
extern FnSetCarBehaviourState RVGL_SetCarBehaviourState;
extern FnSetPostRacePopup RVGL_SetPostRacePopup;

} // namespace Randomizer
