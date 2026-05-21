#pragma once
#include <cstdint>

// ============================================================================
// Addresses_Linux64.h
//
// RVA constants for all hooked and read functions/globals in Linux RVGL 64-bit.
// These are relative virtual addresses: offsets from the start of rvgl.64.
//
// To update after an RVGL update:
//   1. Open the new rvgl.64 in Ghidra.
//   2. Find each function listed below.
//   3. Replace the constant with the new RVA (address shown in Ghidra
//      minus the image base, which is 0x00400000 for this RVGL import).
// ============================================================================

// ----------------------------------------------------------------------------
// Functions
// ----------------------------------------------------------------------------

// LoadVanillaCarPool - loads the built-in car pool into memory.
constexpr uint32_t RVA_LOAD_VANILLA_CAR_POOL   = 0x00047350;

// LoadCustomCarPool - loads the custom car pool into memory.
constexpr uint32_t RVA_LOAD_CUSTOM_CAR_POOL = 0x00047d20;

// CreateCarbox - sets up the frontend textures for car selection.
constexpr uint32_t RVA_CREATE_CARBOX = 0x0014afc0;

// LoadTextureByName - receives a file path and loads a texture to a given slot id.
constexpr uint32_t RVA_LOAD_TEXTURE_BY_NAME = 0x00099550;

// SyncCarInfoFromPhysics - Runs when a car instance is initialized,
// copies parameters back from physics object into CarInfo.
constexpr uint32_t RVA_SYNC_CAR_INFO_FROM_PHYSICS = 0x00044820;

// ParseParametersTxt - Parses the basic metadata when loading a car.
constexpr uint32_t RVA_PARSE_PARAMETERS_TXT = 0x00043f70;

// LoadVanillaTracks - loads the vanilla tracks into memory using the default folder paths.
constexpr uint32_t RVA_LOAD_VANILLA_TRACKS = 0x0005a030;

// LoadCustomTracks - scans the game folders and loads the custom tracks into memory.
constexpr uint32_t RVA_LOAD_CUSTOM_TRACKS = 0x0005a100;

// LoadDefaultCups - loads the vanilla cups into memory using the default folder paths.
constexpr uint32_t RVA_LOAD_VANILLA_CUPS = 0x00052fd0;

// LoadCustomCups - scans the game folders and loads the custom cups into memory.
constexpr uint32_t RVA_LOAD_CUSTOM_CUPS = 0x00053040;

// Cup_InitializeAndValidateAll - validates and updates unlock state for all cups.
constexpr uint32_t RVA_CUP_INITIALIZE_AND_VALIDATE_ALL = 0x00053400;

// Cup_ValidateAndCheckUnlock - validates one cup and updates its isUnlocked flag.
constexpr uint32_t RVA_CUP_VALIDATE_AND_CHECK_UNLOCK = 0x000500a0;

// UpdateCarSelectability - Based on the obtainCondition of each car,
// manipulates the "selectableByPlayer" flag.
constexpr uint32_t RVA_UPDATE_CAR_SELECTABILITY = 0x00044310;

// GetProfileIndex - Returns the index of the profile by name.
constexpr uint32_t RVA_GET_PROFILE_INDEX = 0x0007ae30;

// Profile_CreateOrLoad - Creates or loads a profile by display name.
constexpr uint32_t RVA_PROFILE_CREATE_OR_LOAD = 0x0007ba60;

// Profile_LoadAndReset - Loads a profile and resets state.
constexpr uint32_t RVA_PROFILE_LOAD_AND_RESET = 0x0007b910;

// LoadSettingsFromIni - Loads the INI file and stores settings in global variables in memory.
constexpr uint32_t RVA_LOAD_SETTINGS_FROM_INI = 0x00080a70;

// Track_ApplyCustomUnlock - Update a track's unlock status based on its obtain condition and the player's profile.
constexpr uint32_t RVA_TRACK_APPLY_CUSTOM_UNLOCK = 0x0005b870;

// Track_FileExists - returns true when the track's .inf exists in the VFS.
constexpr uint32_t RVA_TRACK_FILE_EXISTS = 0x0005b620;

// Track_ReversedDirExists - returns true when levels/<folder>/reversed exists.
constexpr uint32_t RVA_TRACK_REVERSED_DIR_EXISTS = 0x0005a4e0;

// CheckIfTierChampionshipWon - returns true if championship for given difficulty has been beaten
constexpr uint32_t RVA_CHECK_IF_TIER_CHAMPIONSHIP_WON = 0x00051980;

// CheckIfTierTimeTrialsBeaten - returns true if normal time trial for all tracks in the given difficulty has been beaten
constexpr uint32_t RVA_CHECK_IF_TIER_TIME_TRIALS_BEATEN = 0x00051700;

// CheckIfTierPracticeStarsFound - returns true if practice stars for all tracks in the given difficulty have been found
constexpr uint32_t RVA_CHECK_IF_TIER_PRACTICE_STARS_FOUND = 0x00051880;

// checkIfTierSingleRacesWon - returns true if single races for all tracks in the given difficulty have been won
constexpr uint32_t RVA_CHECK_IF_TIER_SINGLE_RACES_WON = 0x00051900;

// InitCarPhysicsBlock - Initializes the physics object for a car
constexpr uint32_t RVA_INIT_CAR_PHYSICS_BLOCK = 0x00044a70;

// Token_Matches - String comparison function used when reading from parameters file
constexpr uint32_t RVA_TOKEN_MATCHES = 0x0012a720;

// ReadTokenFloat - Reads a float value from a file
constexpr uint32_t RVA_READ_TOKEN_FLOAT = 0x0012aec0;

// ReadTokenInt - Reads an integer value from a file
constexpr uint32_t RVA_READ_TOKEN_INT = 0x0012b030;

// ReadTokenBool - Reads a boolean value from a file
constexpr uint32_t RVA_READ_TOKEN_BOOL = 0x0012aba0;

// DrawPostRaceLeaderboard - Draws the results table after finishing a race.
constexpr uint32_t RVA_DRAW_POST_RACE_LEADERBOARD = 0x00067da0;

// UpdateTimeTrialLeaderboards - saves time trial leaderboard entries and sets
// challenge progress flags when challenge times are beaten.
constexpr uint32_t RVA_UPDATE_TIME_TRIAL_LEADERBOARDS = 0x0007be50;

// Pickup_CollectProgressObject - handles collectible pickup effects and sets
// practice / Stunt Arena progress.
constexpr uint32_t RVA_PICKUP_COLLECT_PROGRESS_OBJECT = 0x000ca3c0;

// Engine_UpdateRaceProgress - updates lap/race state and sets single-race win progress.
constexpr uint32_t RVA_ENGINE_UPDATE_RACE_PROGRESS = 0x0011cc60;

// Cup_OnStageFinished - handles final cup result and sets championship progress.
constexpr uint32_t RVA_CUP_ON_STAGE_FINISHED = 0x00052d20;

// DrawProgressTable - draws the frontend profile progress table panel.
constexpr uint32_t RVA_DRAW_PROGRESS_TABLE = 0x00148b70;

// Track_LoadProgressFromFile - loads one track's profile progress flags.
constexpr uint32_t RVA_TRACK_LOAD_PROGRESS_FROM_FILE = 0x0007b340;

constexpr uint32_t RVA_RACE_SESSION_SETUP = 0x00057a30;
constexpr uint32_t RVA_SETUP_ALL_RACE_CARS = 0x00056c10;
constexpr uint32_t RVA_RANDOMIZE_CAR_PICKS = 0x00056f40;
constexpr uint32_t RVA_ASSIGN_START_POSITIONS = 0x00056dc0;
constexpr uint32_t RVA_ADD_PARTICIPANT_AND_COUNT = 0x00056690;
constexpr uint32_t RVA_UPDATE_RACE_POSITIONS = 0x00010970;

constexpr uint32_t RVA_CREATE_CAR_ENTITY       = 0x000eab40;
constexpr uint32_t RVA_COMPUTE_SPAWN_ORIENT    = 0x000a93a0;
constexpr uint32_t RVA_SET_CAR_TRANSFORM       = 0x000aa3a0;

constexpr uint32_t RVA_LOAD_OBJECTS_FROM_FOB = 0x000e6bb0;
constexpr uint32_t RVA_CREATE_OBJECT_FROM_FOB = 0x000e69e0;

// Numeric value menu controls
constexpr uint32_t RVA_DRAW_NUMERIC_MENU_VALUE = 0x00140ef0;
constexpr uint32_t RVA_DECREMENT_NUMERIC_MENU_VALUE = 0x0013bd40;
constexpr uint32_t RVA_INCREMENT_NUMERIC_MENU_VALUE = 0x0013bcf0;

// Render functions
constexpr uint32_t RVA_DRAW_UI_TEXT = 0x00095aa0;
constexpr uint32_t RVA_DRAW_SPRITE_2D = 0x00069cb0;
constexpr uint32_t RVA_UI_DRAW_ROUNDED_RECT = 0x0013d350;
constexpr uint32_t RVA_SETUP_GL_RENDER_STATE = 0x000958e0;
constexpr uint32_t RVA_FLUSH_DEFERRED_UI_BATCHES = 0x00088b00;

// Checks whether a track should be selectable/visible in the current frontend mode.
constexpr uint32_t RVA_TRACK_IS_AVAILABLE_FOR_FRONTEND = 0x0005c520;

// Counts visible UTF-8 characters while ignoring combining marks.
constexpr uint32_t RVA_UTF8_VISIBLE_CHAR_COUNT = 0x0012a3d0;


// ----------------------------------------------------------------------------
// Globals
// ----------------------------------------------------------------------------

// Pointer to the CarInfo array.
// Dereference to get the base address of the car pool.
constexpr uint32_t RVA_CAR_TABLE   = 0x002e4ad0;

// Number of cars currently in the pool.
// Includes both vanilla (49) and any loaded custom cars.
constexpr uint32_t RVA_CAR_COUNT   = 0x002e4ad8;

// Pointer table of 49 "cars/<name>" path strings used by LoadVanillaCarPool.
// Each entry points to a null-terminated string in .rodata.
// Override pointers before LoadVanillaCarPool to replace base cars.
constexpr uint32_t RVA_VANILLA_CAR_PATHS = 0x00233c40;

// Pointer to g_VanillaTrackArray.
constexpr uint32_t RVA_VANILLA_TRACKS_TABLE   = 0x002c6960;

// Pointer to g_CustomTrackArray.
constexpr uint32_t RVA_CUSTOM_TRACKS_TABLE   = 0x00b29920;

// Number of tracks currently in the track pool.
// Includes both vanilla (21) and custom tracks.
constexpr uint32_t RVA_TRACK_COUNT   = 0x002cd4b0;

// Pointer to the CupProfile array - with DC data.
// Index 0 is the Bronze Cup, index 1 is the Silver Cup, etc.
constexpr uint32_t RVA_DC_CUP_ARRAY = 0x002c57e0;

// Pointer to the CupProfile array - with vanilla (non-DC) data.
// First entry of the array is empty, index 1 is the Bronze Cup, etc.
constexpr uint32_t RVA_VANILLA_CUP_ARRAY = 0x002c5fe0;

// Global variable for CupDC setting. If enabled, DC Cups are used and DC Cars can be loaded into championships.
constexpr uint32_t RVA_CUP_DC = 0x00b2e41e;

// Number of loaded custom cup profiles.
constexpr uint32_t RVA_CUSTOM_CUPS_COUNT = 0x002e5b40;

// Pointer to the dynamically allocated custom cup profile array.
constexpr uint32_t RVA_CUSTOM_CUP_ARRAY = 0x002e5b48;

// Global variable for the current game mode
constexpr uint32_t RVA_GAME_MODE = 0x002cd4a0;

// Linux direct active profile progress cache block. Name kept for parity with
// Windows, where this constant points to a pointer variable. Do not
// double-dereference this address on Linux. Fields match the Windows pointed-to
// block: +0x10 count, +0x14 max, +0x18 byte array record caught stars.
constexpr uint32_t RVA_TRACK_PROGRESS_CACHE_PTR = 0x00b2d460;

// Controls whether unlock checks are active. 0 means everything is unlocked.
constexpr uint32_t RVA_UNLOCK_CHECKS_ENABLED = 0x002c2c98;

// Car-specific force-unlock flag used by UpdateCarSelectability.
constexpr uint32_t RVA_FORCE_UNLOCK_ALL_CARS = 0x002e55ba;

// Native frontend unlock dialog trigger.
// -1 means no dialog; 0 means generic "new cars delivered" unlock message.
constexpr uint32_t RVA_TIER_UNLOCK_TRIGGER = 0x002c9b80;

// Current Stunt Arena stars earned in the active profile.
constexpr uint32_t RVA_TOTAL_STARS_EARNED = 0x00b2d410;

// Maximum possible Stunt Arena stars for the active profile/content set.
constexpr uint32_t RVA_MAX_POSSIBLE_STARS = 0x00b2d414;

constexpr uint32_t RVA_RACE_FINISHED_FLAG = 0x0aa5b067;

constexpr uint32_t RVA_SETTINGS_NCARS = 0x0aa79d10;
constexpr uint32_t RVA_NCARS = 0x00b2e408;
constexpr uint32_t RVA_SETTINGS_NCARS_MENU_ITEM = 0x002cad60;

// Current frontend menu action code. Values 2/3 are left/right.
constexpr uint32_t RVA_MENU_ACTION = 0x002ca2b4;

// Pointer to the active locale string table.
constexpr uint32_t RVA_LOCALE_STRINGS_PTR = 0x0aa7a3c0;

// Pointer to frontend menu slot runtime storage.
constexpr uint32_t RVA_MENU_SLOTS_PTR = 0x0aa79de0;

constexpr uint32_t RVA_RACE_PARTICIPANT_COUNT = 0x00b28a84;
constexpr uint32_t RVA_RACE_PARTICIPANT_RECORDS = 0x00b28ad0;

// Head of the active CarEntity linked list. Each node is a live car instance;
// this is separate from the CarInfo model metadata table.
constexpr uint32_t RVA_CAR_LIST_HEAD = 0x0a8d9e00;


// ----------------------------------------------------------------------------
// AbsFromRva
//
// Converts a compile-time RVA constant to the actual runtime address by
// adding the base address RVGL was loaded at. Call this at hook installation
// time, not at compile time, because the module base is only known once
// the process is running.
// ----------------------------------------------------------------------------
