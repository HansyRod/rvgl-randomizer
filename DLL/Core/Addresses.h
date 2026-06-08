#pragma once
#include <cstdint>
#include <windows.h>

// ============================================================================
// Addresses.h
//
// RVA constants for all hooked and read functions/globals in RVGL.
// These are relative virtual addresses — offsets from the start of rvgl.exe.
//
// To update after an RVGL update:
//   1. Open the new rvgl.exe in Ghidra.
//   2. Find each function listed below.
//   3. Replace the constant with the new RVA (address shown in Ghidra
//      minus the image base, which is 0x00400000 for RVGL).
// ============================================================================

// ----------------------------------------------------------------------------
// Functions
// ----------------------------------------------------------------------------

// LoadVanillaCarPool — loads the built-in car pool into memory.
// Ghidra signature: bool __fastcall LoadVanillaCarPool(void)
constexpr uint32_t RVA_LOAD_VANILLA_CAR_POOL   = 0x0003F140;

// LoadCustomCarPool — loads the custom car pool into memory.
// Ghidra signature: void LoadCustomCarPool (void)
constexpr uint32_t RVA_LOAD_CUSTOM_CAR_POOL = 0x0003fac0;

// CreateCarbox — sets up the frontend textures for car selection.
// Ghidra signature: void __fastcall CreateCarbox(void* param_1)
constexpr uint32_t RVA_CREATE_CARBOX = 0x001570d0;

// LoadTextureByName - receives a file path and loads a texture to a given slot id.
constexpr uint32_t RVA_LOAD_TEXTURE_BY_NAME = 0x000962d0;

// SyncCarInfoFromPhysics - Runs when a car instance is initialized,
// copies parameters back from physics object into CarInfo.
constexpr uint32_t RVA_SYNC_CAR_INFO_FROM_PHYSICS = 0x0003c330;

// ParseParametersTxt - Parses the basic metadata when loading a car.
constexpr uint32_t RVA_PARSE_PARAMETERS_TXT = 0x0003b6c0;

// LoadVanillaTracks — loads the vanilla tracks into memory using the default folder paths.
// Ghidra signature: void LoadVanillaTracks(void)
constexpr uint32_t RVA_LOAD_VANILLA_TRACKS = 0x00052190;

// LoadCustomTracks — scans the game folders and loads the custom tracks into memory.
// Ghidra signature: void LoadCustomTracks(void)
constexpr uint32_t RVA_LOAD_CUSTOM_TRACKS = 0x00052280;

// LoadDefaultCups — loads the vanilla cups into memory using the default folder paths.
// Ghidra signature: void LoadDefaultCups(void)
constexpr uint32_t RVA_LOAD_VANILLA_CUPS = 0x0004bae0;

// LoadCustomCups — scans the game folders and loads the custom cups into memory.
// Ghidra signature: void LoadCustomCups(void)
constexpr uint32_t RVA_LOAD_CUSTOM_CUPS = 0x0004bb80;

// Cup_InitializeAndValidateAll - validates and updates unlock state for all cups.
constexpr uint32_t RVA_CUP_INITIALIZE_AND_VALIDATE_ALL = 0x0004bf30;

// Cup_ValidateAndCheckUnlock - validates one cup and updates its isUnlocked flag.
constexpr uint32_t RVA_CUP_VALIDATE_AND_CHECK_UNLOCK = 0x00048840;

// UpdateCarSelectability - Based on the obtainCondition of each car,
// manipulates the "selectableByPlayer" flag.
constexpr uint32_t RVA_UPDATE_CAR_SELECTABILITY = 0x0003bdf0;

// GetProfileIndex - Returns the index of the profile by name.
constexpr uint32_t RVA_GET_PROFILE_INDEX = 0x00074890;

// Profile_CreateOrLoad - Creates or loads a profile by display name.
constexpr uint32_t RVA_PROFILE_CREATE_OR_LOAD = 0x00075500;

// Profile_LoadAndReset - Loads a profile and resets state.
constexpr uint32_t RVA_PROFILE_LOAD_AND_RESET = 0x000753c0;

// LoadSettingsFromIni - Loads the INI file and stores settings in global variables in memory.
constexpr uint32_t RVA_LOAD_SETTINGS_FROM_INI = 0x0007a6a0;

// Track_ApplyCustomUnlock - Update a track's unlock status based on its obtain condition and the player's profile.
constexpr uint32_t RVA_TRACK_APPLY_CUSTOM_UNLOCK = 0x00053ba0;

// Track_FileExists - returns true when the track's .inf exists in the VFS.
constexpr uint32_t RVA_TRACK_FILE_EXISTS = 0x00053470;

// Track_ReversedDirExists - returns true when levels/<folder>/reversed exists.
constexpr uint32_t RVA_TRACK_REVERSED_DIR_EXISTS = 0x000526a0;

// CheckIfTierChampionshipWon - returns true if championship for given difficulty has been beaten
constexpr uint32_t RVA_CHECK_IF_TIER_CHAMPIONSHIP_WON = 0x0004a250;

// CheckIfTierTimeTrialsBeaten - returns true if normal time trial for all tracks in the given difficulty has been beaten
constexpr uint32_t RVA_CHECK_IF_TIER_TIME_TRIALS_BEATEN = 0x00049f50;

// CheckIfTierPracticeStarsFound - returns true if practice stars for all tracks in the given difficulty have been found
constexpr uint32_t RVA_CHECK_IF_TIER_PRACTICE_STARS_FOUND = 0x0004a110;

// checkIfTierSingleRacesWon - returns true if single races for all tracks in the given difficulty have been won
constexpr uint32_t RVA_CHECK_IF_TIER_SINGLE_RACES_WON = 0x0004a1b0;

// InitCarPhysicsBlock - Initializes the physics object for a car
constexpr uint32_t RVA_INIT_CAR_PHYSICS_BLOCK = 0x0003c580;

// Token_Matches - String comparison function used when reading from parameters file
constexpr uint32_t RVA_TOKEN_MATCHES = 0x00133ef0;

// ReadTokenFloat - Reads a float value from a file
constexpr uint32_t RVA_READ_TOKEN_FLOAT = 0x001345e0;

// ReadTokenInt - Reads an integer value from a file
constexpr uint32_t RVA_READ_TOKEN_INT = 0x001346e0;

// ReadTokenBool - Reads a boolean value from a file
constexpr uint32_t RVA_READ_TOKEN_BOOL = 0x001343c0;

// DrawPostRaceLeaderboard - Draws the results table after finishing a race.
constexpr uint32_t RVA_DRAW_POST_RACE_LEADERBOARD = 0x00061370;

// UpdateTimeTrialLeaderboards - saves time trial leaderboard entries and sets
// challenge progress flags when challenge times are beaten.
constexpr uint32_t RVA_UPDATE_TIME_TRIAL_LEADERBOARDS = 0x00075900;

// Pickup_CollectProgressObject - handles collectible pickup effects and sets
// practice / Stunt Arena progress.
constexpr uint32_t RVA_PICKUP_COLLECT_PROGRESS_OBJECT = 0x000CD420;

// Engine_UpdateRaceProgress - updates lap/race state and sets single-race win progress.
constexpr uint32_t RVA_ENGINE_UPDATE_RACE_PROGRESS = 0x001257E0;

// Cup_OnStageFinished - handles final cup result and sets championship progress.
constexpr uint32_t RVA_CUP_ON_STAGE_FINISHED = 0x0004B800;

// Cup_GenerateOpponentGrid - builds the persistent championship participant table.
constexpr uint32_t RVA_CUP_GENERATE_OPPONENT_GRID = 0x000493D0;

// BuildGrid - prepares the next championship race and adds its participants.
constexpr uint32_t RVA_BUILD_GRID = 0x00049DC0;

// UpdateCupPostRaceProgress - championship post-race points/standings state machine.
constexpr uint32_t RVA_UPDATE_CUP_POST_RACE_PROGRESS = 0x0004A420;

// DrawCupStandingsTable - draws the championship post-race standings table.
constexpr uint32_t RVA_DRAW_CUP_STANDINGS_TABLE = 0x00060730;

// ResetCurrentTrackSelectionState - clears the active track folder and participant counters.
constexpr uint32_t RVA_RESET_CURRENT_TRACK_SELECTION_STATE = 0x0004F350;

// Race_TeardownAndSave - tears down the active race and saves progress/session output.
constexpr uint32_t RVA_RACE_TEARDOWN_AND_SAVE = 0x00055810;

// Level_DestroyAndFree - destroys and frees the loaded level.
constexpr uint32_t RVA_LEVEL_DESTROY_AND_FREE = 0x00055330;

// Loads the next race from PlayerRaceInfo after cup stage transitions.
constexpr uint32_t RVA_LOAD_NEXT_RACE_FROM_PLAYER_RACE_INFO = 0x00006B40;

// DrawProgressTable - draws the frontend profile progress table panel.
constexpr uint32_t RVA_DRAW_PROGRESS_TABLE = 0x00154870;

// Start-race frontend menu builder and action handler.
constexpr uint32_t RVA_BUILD_START_RACE_MENU = 0x0015E730;
constexpr uint32_t RVA_HANDLE_START_RACE_MENU_ACTION = 0x0015E880;

// Options frontend menu builder.
constexpr uint32_t RVA_BUILD_OPTIONS_MENU = 0x00154700;

// Registers one MenuItemDescriptor in the currently active frontend menu slot.
constexpr uint32_t RVA_REGISTER_MENU_ITEM_IN_ACTIVE_MENU = 0x001453B0;

// Shared frontend menu action handler for up/down/left/right/back navigation.
constexpr uint32_t RVA_HANDLE_GENERIC_MENU_ACTION = 0x00146CF0;

// RegisterFinishTime - adds a car to the 30-entry race result table.
constexpr uint32_t RVA_REGISTER_FINISH_TIME = 0x000F0890;

// SetCarBehaviourState - switches a live car entity between native behavior states.
constexpr uint32_t RVA_SET_CAR_BEHAVIOUR_STATE = 0x000EFD50;

// UI_SetPostRacePopup - shows the bottom-center race finish notification.
constexpr uint32_t RVA_UI_SET_POST_RACE_POPUP = 0x00068200;

// Track_LoadProgressFromFile - loads one track's profile progress flags.
constexpr uint32_t RVA_TRACK_LOAD_PROGRESS_FROM_FILE = 0x00074db0;

constexpr uint32_t RVA_RACE_SESSION_SETUP = 0x00050700;
constexpr uint32_t RVA_SETUP_ALL_RACE_CARS = 0x0004F7B0;
constexpr uint32_t RVA_RANDOMIZE_CAR_PICKS = 0x0004FB30;
constexpr uint32_t RVA_ASSIGN_START_POSITIONS = 0x0004F980;
constexpr uint32_t RVA_ADD_PARTICIPANT_AND_COUNT = 0x0004f1f0;
constexpr uint32_t RVA_UPDATE_RACE_POSITIONS = 0x000075F0;

constexpr uint32_t RVA_CREATE_CAR_ENTITY       = 0x000F03A0;
constexpr uint32_t RVA_COMPUTE_SPAWN_ORIENT    = 0x000A71D0;
constexpr uint32_t RVA_SET_CAR_TRANSFORM       = 0x000A76F0;

constexpr uint32_t RVA_LOAD_OBJECTS_FROM_FOB = 0x000eb940;
constexpr uint32_t RVA_CREATE_OBJECT_FROM_FOB = 0x000eb740;

// Numeric value menu controls
constexpr uint32_t RVA_DRAW_NUMERIC_MENU_VALUE = 0x0014c2e0;
constexpr uint32_t RVA_DECREMENT_NUMERIC_MENU_VALUE = 0x00147000;
constexpr uint32_t RVA_INCREMENT_NUMERIC_MENU_VALUE = 0x00146fb0;

// Render functions
constexpr uint32_t RVA_DRAW_UI_TEXT = 0x00092150;
constexpr uint32_t RVA_DRAW_SPRITE_2D = 0x000628b0;
constexpr uint32_t RVA_UI_DRAW_ROUNDED_RECT = 0x00148640;
constexpr uint32_t RVA_SETUP_GL_RENDER_STATE = 0x00091f60;
constexpr uint32_t RVA_FLUSH_DEFERRED_UI_BATCHES = 0x00082e40;

// Checks whether a track should be selectable/visible in the current frontend mode.
constexpr uint32_t RVA_TRACK_IS_AVAILABLE_FOR_FRONTEND = 0x000548d0;

// Counts visible UTF-8 characters while ignoring combining marks.
constexpr uint32_t RVA_UTF8_VISIBLE_CHAR_COUNT = 0x00133aa0;


// ----------------------------------------------------------------------------
// Globals
// ----------------------------------------------------------------------------

// Pointer to the CarInfo array (DAT_006fab50).
// Dereference to get the base address of the car pool.
constexpr uint32_t RVA_CAR_TABLE   = 0x002FAB50;

// Number of cars currently in the pool (DAT_006fab58).
// Includes both vanilla (49) and any loaded custom cars.
constexpr uint32_t RVA_CAR_COUNT   = 0x002FAB58;

// Pointer table of 49 "cars/<name>" path strings used by LoadVanillaCarPool.
// Each entry points to a null-terminated string in .rdata.
// Override pointers before LoadVanillaCarPool to replace base cars.
constexpr uint32_t RVA_VANILLA_CAR_PATHS = 0x002720a0;

// Pointer to g_VanillaTrackArray (DAT_0065fe20).
constexpr uint32_t RVA_VANILLA_TRACKS_TABLE   = 0x0025fe20;

// Pointer to g_CustomTrackArray (DAT_00f3f9a0).
constexpr uint32_t RVA_CUSTOM_TRACKS_TABLE   = 0x00b3f9a0;

// Number of tracks currently in the track pool.
// Includes both vanilla (21) and custom tracks.
constexpr uint32_t RVA_TRACK_COUNT   = 0x002e34d0;

// Pointer to the CupProfile array - with DC data.
// Index 0 is the Bronze Cup, index 1 is the Silver Cup, etc.
constexpr uint32_t RVA_DC_CUP_ARRAY = 0x0025ee38;

// Pointer to the CupProfile array - with vanilla (non-DC) data.
// First entry of the array is empty, index 1 is the Bronze Cup, etc.
constexpr uint32_t RVA_VANILLA_CUP_ARRAY = 0x0025f4a0;

// Global variable for CupDC setting. If enabled, DC Cups are used and DC Cars can be loaded into championships.
constexpr uint32_t RVA_CUP_DC = 0x00b4349e;

// Global setting that enables or disables random skins.
constexpr uint32_t RVA_RANDOM_SKINS_ENABLED = 0x00b434a2;

// Number of loaded custom cup profiles.
constexpr uint32_t RVA_CUSTOM_CUPS_COUNT = 0x002fbbc0;

// Pointer to the dynamically allocated custom cup profile array.
constexpr uint32_t RVA_CUSTOM_CUP_ARRAY = 0x002fbbc8;

// Global variable for the current game mode
constexpr uint32_t RVA_GAME_MODE = 0x002e34c0;

// Pointer to the active profile progress cache. In Stunt Arena this points to
// a block whose +0x10 count, +0x14 max, and +0x18 byte array record caught stars.
constexpr uint32_t RVA_TRACK_PROGRESS_CACHE_PTR = 0x002a7750;

// Controls whether unlock checks are active. 0 means everything is unlocked.
constexpr uint32_t RVA_UNLOCK_CHECKS_ENABLED = 0x0025c0f8;

// Car-specific force-unlock flag used by UpdateCarSelectability.
constexpr uint32_t RVA_FORCE_UNLOCK_ALL_CARS = 0x002fb64a;

// Native frontend unlock dialog trigger.
// -1 means no dialog; 0 means generic "new cars delivered" unlock message.
constexpr uint32_t RVA_TIER_UNLOCK_TRIGGER = 0x002630a0;

// Current Stunt Arena stars earned in the active profile.
constexpr uint32_t RVA_TOTAL_STARS_EARNED = 0x00b424f0;

// Maximum possible Stunt Arena stars for the active profile/content set.
constexpr uint32_t RVA_MAX_POSSIBLE_STARS = 0x00b42494;

constexpr uint32_t RVA_RACE_FINISHED_FLAG = 0x0a8ee7e0;

// Pointer globals used by in-race results and finish notifications.
constexpr uint32_t RVA_CURRENT_RACE_CLOCK_MS_PTR = 0x002a79d0;
constexpr uint32_t RVA_RACE_RESULT_TABLE_PTR = 0x002a7400;
constexpr uint32_t RVA_PLAYER_COLORS_PTR = 0x002a6d90;

constexpr uint32_t RVA_SETTINGS_NCARS = 0x0aa8e930;
constexpr uint32_t RVA_NCARS = 0x00b43488;
constexpr uint32_t RVA_SETTINGS_NCARS_MENU_ITEM = 0x00264280;

// Current frontend menu action code. Values 2/3 are left/right.
constexpr uint32_t RVA_MENU_ACTION = 0x002637f4;

// Pointer to the active locale string table.
constexpr uint32_t RVA_LOCALE_STRINGS_PTR = 0x002a79a0;

// Pointer to frontend menu slot runtime storage.
constexpr uint32_t RVA_MENU_SLOTS_PTR = 0x002a6860;

// Pointer-to-pointer to the active UI viewport transform. Native cup progress
// drawing reads *(*ptr + 0x10) and applies DAT_006755ec before subtracting 320.
constexpr uint32_t RVA_UI_VIEWPORT_PTR_PTR = 0x002a5f50;

// Native DrawCupStandingsTable horizontal coordinate scale (DAT_006755ec).
constexpr uint32_t RVA_CUP_PROGRESS_UI_COORD_SCALE = 0x002755ec;

// Pointer to the active post-race/menu display state. Native cup progress table is visible at state 4.
constexpr uint32_t RVA_POST_RACE_MENU_DISPLAY_STATE_PTR = 0x002a5da0;

constexpr uint32_t RVA_RACE_PARTICIPANT_COUNT = 0x00b3eb04;
constexpr uint32_t RVA_RACE_PARTICIPANT_RECORDS = 0x00b3eb50;

// Head of the active CarEntity linked list. Each node is a live car instance;
// this is separate from the CarInfo model metadata table.
constexpr uint32_t RVA_CAR_LIST_HEAD = 0x0a8ee9e8;

// Pointer to the active race settings block used by race setup.
constexpr uint32_t RVA_RACE_SETTINGS_PTR = 0x002a7910;

// Pointer to the active player/race info block populated by BuildGrid.
constexpr uint32_t RVA_PLAYER_RACE_INFO_PTR = 0x002a7770;

// Pointer to the global car-model half-scale flag copied by native BuildGrid
// into PlayerRaceInfo +0x34.
constexpr uint32_t RVA_DRINKME_ENABLED_PTR = 0x002a5e50;

// Pointer variable holding the active CupProfile during a championship.
constexpr uint32_t RVA_ACTIVE_CUP_PTR = 0x0025ec60;

// Native championship runtime state.
constexpr uint32_t RVA_CURRENT_CUP_INDEX = 0x002fbbe0;
constexpr uint32_t RVA_CURRENT_CUP_STAGE_INDEX = 0x002fbbe4;
constexpr uint32_t RVA_CUP_TRIES_LEFT = 0x002fbbe8;
constexpr uint32_t RVA_CUP_STAGE_DIRECTION = 0x002fbbec;
constexpr uint32_t RVA_CUP_POST_RACE_STATE = 0x002fbbd0;
constexpr uint32_t RVA_CUP_RESULT = 0x002fce70;
constexpr uint32_t RVA_NATIVE_CUP_PARTICIPANTS = 0x002fbbf0;
constexpr uint32_t RVA_NATIVE_CUP_STANDINGS_SORTED = 0x002fc530;

// Pointer globals used when returning from a completed cup to the frontend.
constexpr uint32_t RVA_FRONTEND_CUP_RESULT_FLAG_PTR = 0x002a7470;
constexpr uint32_t RVA_GAME_STATE_FUNCTION_PTR = 0x002a62c0;
constexpr uint32_t RVA_MENU_INITIALIZE_FRONTEND_PTR = 0x002a47f0;





// ----------------------------------------------------------------------------
// AbsFromRva
//
// Converts a compile-time RVA constant to the actual runtime address by
// adding the base address RVGL was loaded at. Call this at hook installation
// time — not at compile time — because the module base is only known once
// the process is running.
// ----------------------------------------------------------------------------
inline uintptr_t AbsFromRva(uint32_t rva) {
    return reinterpret_cast<uintptr_t>(GetModuleHandleA("rvgl.exe")) + rva;
}
