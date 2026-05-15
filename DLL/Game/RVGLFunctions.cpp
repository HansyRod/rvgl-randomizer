#include "RVGLFunctions.h"
#include "Addresses.h"

namespace Randomizer {

FnCreateCarEntity           RVGL_CreateCarEntity            = reinterpret_cast<FnCreateCarEntity>(AbsFromRva(RVA_CREATE_CAR_ENTITY));
FnComputeSpawnOrientation   RVGL_ComputeSpawnOrientation    = reinterpret_cast<FnComputeSpawnOrientation>(AbsFromRva(RVA_COMPUTE_SPAWN_ORIENT));
FnSetCarTransform           RVGL_SetCarTransform            = reinterpret_cast<FnSetCarTransform>(AbsFromRva(RVA_SET_CAR_TRANSFORM));
FnDrawNumericMenuValue      RVGL_DrawNumericMenuValue       = reinterpret_cast<FnDrawNumericMenuValue>(AbsFromRva(RVA_DRAW_NUMERIC_MENU_VALUE));
FnDecrementNumericMenuValue RVGL_DecrementNumericMenuValue  = reinterpret_cast<FnDecrementNumericMenuValue>(AbsFromRva(RVA_DECREMENT_NUMERIC_MENU_VALUE));
FnIncrementNumericMenuValue RVGL_IncrementNumericMenuValue  = reinterpret_cast<FnIncrementNumericMenuValue>(AbsFromRva(RVA_INCREMENT_NUMERIC_MENU_VALUE));
FnCreateObjectFromFob       RVGL_CreateObjectFromFob        = reinterpret_cast<FnCreateObjectFromFob>(AbsFromRva(RVA_CREATE_OBJECT_FROM_FOB));
FnTrackFileExists           RVGL_TrackFileExists            = reinterpret_cast<FnTrackFileExists>(AbsFromRva(RVA_TRACK_FILE_EXISTS));
FnTrackReversedDirExists    RVGL_TrackReversedDirExists     = reinterpret_cast<FnTrackReversedDirExists>(AbsFromRva(RVA_TRACK_REVERSED_DIR_EXISTS));
FnTrackLoadProgressFromFile RVGL_TrackLoadProgressFromFile  = reinterpret_cast<FnTrackLoadProgressFromFile>(AbsFromRva(RVA_TRACK_LOAD_PROGRESS_FROM_FILE));
FnTrackIsAvailableForFrontend RVGL_TrackIsAvailableForFrontend = reinterpret_cast<FnTrackIsAvailableForFrontend>(AbsFromRva(RVA_TRACK_IS_AVAILABLE_FOR_FRONTEND));
FnDrawTexturedQuad          RVGL_DrawTexturedQuad           = reinterpret_cast<FnDrawTexturedQuad>(AbsFromRva(RVA_DRAW_TEXTURED_QUAD));
FnDrawSprite2D              RVGL_DrawSprite2D               = reinterpret_cast<FnDrawSprite2D>(AbsFromRva(RVA_DRAW_SPRITE_2D));
FnUIDrawRoundedRect         RVGL_UIDrawRoundedRect          = reinterpret_cast<FnUIDrawRoundedRect>(AbsFromRva(RVA_UI_DRAW_ROUNDED_RECT));
FnSetupGLRenderState        RVGL_SetupGLRenderState         = reinterpret_cast<FnSetupGLRenderState>(AbsFromRva(RVA_SETUP_GL_RENDER_STATE));
FnNetworkRenderStub         RVGL_NetworkRenderStub          = reinterpret_cast<FnNetworkRenderStub>(AbsFromRva(RVA_NETWORK_RENDER_STUB));
FnUTF8GetVisibleCharCount   RVGL_UTF8GetVisibleCharCount    = reinterpret_cast<FnUTF8GetVisibleCharCount>(AbsFromRva(RVA_UTF8_VISIBLE_CHAR_COUNT));

} // namespace Randomizer
