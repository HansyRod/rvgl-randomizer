#include "RVGLFunctions.h"
#include "Addresses.h"

namespace Randomizer {

FnCreateCarEntity           RVGL_CreateCarEntity            = reinterpret_cast<FnCreateCarEntity>(AbsFromRva(RVA_CREATE_CAR_ENTITY));
FnComputeSpawnOrientation   RVGL_ComputeSpawnOrientation    = reinterpret_cast<FnComputeSpawnOrientation>(AbsFromRva(RVA_COMPUTE_SPAWN_ORIENT));
FnSetCarTransform           RVGL_SetCarTransform            = reinterpret_cast<FnSetCarTransform>(AbsFromRva(RVA_SET_CAR_TRANSFORM));
FnGetCollisionGridCellIndex RVGL_GetCollisionGridCellIndex  = reinterpret_cast<FnGetCollisionGridCellIndex>(AbsFromRva(RVA_GET_COLLISION_GRID_CELL_INDEX));
FnTraceSegmentAgainstCollisionGrid RVGL_TraceSegmentAgainstCollisionGrid = reinterpret_cast<FnTraceSegmentAgainstCollisionGrid>(AbsFromRva(RVA_TRACE_SEGMENT_COLLISION_GRID));
FnFindTrackZoneForCarBruteForce RVGL_FindTrackZoneForCarBruteForce = reinterpret_cast<FnFindTrackZoneForCarBruteForce>(AbsFromRva(RVA_FIND_TRACK_ZONE_FOR_CAR_BRUTE_FORCE));
FnDrawNumericMenuValue      RVGL_DrawNumericMenuValue       = reinterpret_cast<FnDrawNumericMenuValue>(AbsFromRva(RVA_DRAW_NUMERIC_MENU_VALUE));
FnDecrementNumericMenuValue RVGL_DecrementNumericMenuValue  = reinterpret_cast<FnDecrementNumericMenuValue>(AbsFromRva(RVA_DECREMENT_NUMERIC_MENU_VALUE));
FnIncrementNumericMenuValue RVGL_IncrementNumericMenuValue  = reinterpret_cast<FnIncrementNumericMenuValue>(AbsFromRva(RVA_INCREMENT_NUMERIC_MENU_VALUE));
FnCreateObjectFromFob       RVGL_CreateObjectFromFob        = reinterpret_cast<FnCreateObjectFromFob>(AbsFromRva(RVA_CREATE_OBJECT_FROM_FOB));
FnVfsResolvePath            RVGL_VfsResolvePath             = reinterpret_cast<FnVfsResolvePath>(AbsFromRva(RVA_VFS_RESOLVE_PATH));
FnTrackFileExists           RVGL_TrackFileExists            = reinterpret_cast<FnTrackFileExists>(AbsFromRva(RVA_TRACK_FILE_EXISTS));
FnTrackReversedDirExists    RVGL_TrackReversedDirExists     = reinterpret_cast<FnTrackReversedDirExists>(AbsFromRva(RVA_TRACK_REVERSED_DIR_EXISTS));
FnTrackLoadProgressFromFile RVGL_TrackLoadProgressFromFile  = reinterpret_cast<FnTrackLoadProgressFromFile>(AbsFromRva(RVA_TRACK_LOAD_PROGRESS_FROM_FILE));
FnTrackIsAvailableForFrontend RVGL_TrackIsAvailableForFrontend = reinterpret_cast<FnTrackIsAvailableForFrontend>(AbsFromRva(RVA_TRACK_IS_AVAILABLE_FOR_FRONTEND));
FnDrawUIText                RVGL_DrawUIText                 = reinterpret_cast<FnDrawUIText>(AbsFromRva(RVA_DRAW_UI_TEXT));
FnDrawSprite2D              RVGL_DrawSprite2D               = reinterpret_cast<FnDrawSprite2D>(AbsFromRva(RVA_DRAW_SPRITE_2D));
FnUIDrawRoundedRect         RVGL_UIDrawRoundedRect          = reinterpret_cast<FnUIDrawRoundedRect>(AbsFromRva(RVA_UI_DRAW_ROUNDED_RECT));
FnSetupGLRenderState        RVGL_SetupGLRenderState         = reinterpret_cast<FnSetupGLRenderState>(AbsFromRva(RVA_SETUP_GL_RENDER_STATE));
FnFlushDeferredUIBatches    RVGL_FlushDeferredUIBatches     = reinterpret_cast<FnFlushDeferredUIBatches>(AbsFromRva(RVA_FLUSH_DEFERRED_UI_BATCHES));
FnUTF8GetVisibleCharCount   RVGL_UTF8GetVisibleCharCount    = reinterpret_cast<FnUTF8GetVisibleCharCount>(AbsFromRva(RVA_UTF8_VISIBLE_CHAR_COUNT));
FnStringLastCharPos         RVGL_StringLastCharPos          = reinterpret_cast<FnStringLastCharPos>(AbsFromRva(RVA_STRING_LAST_CHAR_POS));
FnUTF8Check                 RVGL_UTF8Check                  = reinterpret_cast<FnUTF8Check>(AbsFromRva(RVA_UTF8_CHECK));
FnResetCurrentTrackSelectionState RVGL_ResetCurrentTrackSelectionState = reinterpret_cast<FnResetCurrentTrackSelectionState>(AbsFromRva(RVA_RESET_CURRENT_TRACK_SELECTION_STATE));
FnRaceTeardownAndSave       RVGL_RaceTeardownAndSave        = reinterpret_cast<FnRaceTeardownAndSave>(AbsFromRva(RVA_RACE_TEARDOWN_AND_SAVE));
FnLevelDestroyAndFree       RVGL_LevelDestroyAndFree        = reinterpret_cast<FnLevelDestroyAndFree>(AbsFromRva(RVA_LEVEL_DESTROY_AND_FREE));
FnLoadNextRaceFromPlayerRaceInfo RVGL_LoadNextRaceFromPlayerRaceInfo = reinterpret_cast<FnLoadNextRaceFromPlayerRaceInfo>(AbsFromRva(RVA_LOAD_NEXT_RACE_FROM_PLAYER_RACE_INFO));
FnRegisterMenuItemInActiveMenu RVGL_RegisterMenuItemInActiveMenu = reinterpret_cast<FnRegisterMenuItemInActiveMenu>(AbsFromRva(RVA_REGISTER_MENU_ITEM_IN_ACTIVE_MENU));
FnActivateMenuDisplaySlot   RVGL_ActivateMenuDisplaySlot    = reinterpret_cast<FnActivateMenuDisplaySlot>(AbsFromRva(RVA_ACTIVATE_MENU_DISPLAY_SLOT));
FnHandleGenericMenuAction   RVGL_HandleGenericMenuAction    = reinterpret_cast<FnHandleGenericMenuAction>(AbsFromRva(RVA_HANDLE_GENERIC_MENU_ACTION));
FnInputGetTextInput         RVGL_InputGetTextInput          = reinterpret_cast<FnInputGetTextInput>(AbsFromRva(RVA_INPUT_GET_TEXT_INPUT));
FnSDLGetKeyboardState       RVGL_SDLGetKeyboardState        = reinterpret_cast<FnSDLGetKeyboardState>(AbsFromRva(RVA_SDL_GET_KEYBOARD_STATE));
FnSDLGetModState            RVGL_SDLGetModState             = reinterpret_cast<FnSDLGetModState>(AbsFromRva(RVA_SDL_GET_MOD_STATE));
FnSDLGetClipboardText       RVGL_SDLGetClipboardText        = reinterpret_cast<FnSDLGetClipboardText>(AbsFromRva(RVA_SDL_GET_CLIPBOARD_TEXT));
FnSDLFree                   RVGL_SDLFree                    = reinterpret_cast<FnSDLFree>(AbsFromRva(RVA_SDL_FREE));
FnRegisterFinishTime        RVGL_RegisterFinishTime         = reinterpret_cast<FnRegisterFinishTime>(AbsFromRva(RVA_REGISTER_FINISH_TIME));
FnSetCarBehaviourState      RVGL_SetCarBehaviourState       = reinterpret_cast<FnSetCarBehaviourState>(AbsFromRva(RVA_SET_CAR_BEHAVIOUR_STATE));
FnSetPostRacePopup          RVGL_SetPostRacePopup           = reinterpret_cast<FnSetPostRacePopup>(AbsFromRva(RVA_UI_SET_POST_RACE_POPUP));

} // namespace Randomizer
