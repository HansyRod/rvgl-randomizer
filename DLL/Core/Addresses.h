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

// Global variable for the current game mode
constexpr uint32_t RVA_GAME_MODE = 0x002e34c0;

// Current Stunt Arena stars earned in the active profile.
constexpr uint32_t RVA_TOTAL_STARS_EARNED = 0x00b42494;

// Maximum possible Stunt Arena stars for the active profile/content set.
constexpr uint32_t RVA_MAX_POSSIBLE_STARS = 0x00b42490;

constexpr uint32_t RVA_RACE_FINISHED_FLAG = 0x0a8ee7e0;

constexpr uint32_t RVA_SETTINGS_NCARS = 0x0aa8e930;
constexpr uint32_t RVA_NCARS = 0x00b43488;
constexpr uint32_t RVA_SETTINGS_NCARS_MENU_ITEM = 0x00264280;

constexpr uint32_t RVA_RACE_PARTICIPANT_COUNT = 0x00b3eb04;
constexpr uint32_t RVA_RACE_PARTICIPANT_RECORDS = 0x00b3eb50;

// Head of the active CarEntity linked list. Each node is a live car instance;
// this is separate from the CarInfo model metadata table.
constexpr uint32_t RVA_CAR_LIST_HEAD = 0x0a8ee9e8;

// Numeric value menu controls
constexpr uint32_t RVA_DRAW_NUMERIC_MENU_VALUE = 0x0014c2e0;
constexpr uint32_t RVA_DECREMENT_NUMERIC_MENU_VALUE = 0x00147000;
constexpr uint32_t RVA_INCREMENT_NUMERIC_MENU_VALUE = 0x00146fb0;




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
