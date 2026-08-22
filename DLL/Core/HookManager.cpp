#include "HookManager.h"
#include "MinHook.h"
#include "Addresses.h"
#include "CarHooks.h"
#include "TrackHooks.h"
#include "CupHooks.h"
#include "ProfileHooks.h"
#include "ProgressTableHooks.h"
#include "ProgressFlagHooks.h"
#include "CarPhysicsHooks.h"
#include "RaceInitHooks.h"
#include "CupOpponentGrid.h"
#include "ThirtyCarCupMod.h"
#include "MenuMod.h"
#include "Fob.h"
#include "CallLogger.h"
#include "Logger.h"
#include <string>
#include <vector>

// ============================================================================
// Internal state
// ============================================================================

namespace {

struct HookEntry {
    uintptr_t   target;
    void*       detour;
    void**      original;
    const char* name;
    bool        installed;
};

std::vector<HookEntry> g_hooks;
bool                   g_minHookInitialized = false;

} // anonymous namespace

// ============================================================================
// HookManager::Add  (typed hooks — when you need to inspect args or modify
//                    behaviour, not just log the call)
// ============================================================================

bool HookManager::Add(uintptr_t target, void* detour, void** original, const char* name) {
    if (!target) {
        // Address resolved to zero — RVA is wrong or module base not found.
        Logger::TimestampLogf("[HookManager] SKIP (zero address): %s", name);
        return false;
    }

    MH_STATUS status = MH_CreateHook(reinterpret_cast<void*>(target), detour, original);

    if (status != MH_OK && status != MH_ERROR_ALREADY_CREATED) {
        Logger::TimestampLogf("[HookManager] FAIL MH_CreateHook: %s — %s",
                     name, MH_StatusToString(status));
        return false;
    }

    status = MH_EnableHook(reinterpret_cast<void*>(target));

    if (status != MH_OK && status != MH_ERROR_ENABLED) {
        Logger::TimestampLogf("[HookManager] FAIL MH_EnableHook: %s — %s",
                     name, MH_StatusToString(status));
        return false;
    }

    g_hooks.push_back({ target, detour, original, name, true });
    Logger::TimestampLogf("[HookManager] OK: %s", name);
    return true;
}

// ============================================================================
// Hook list
//
// This is the only place that knows which RVGL functions are intercepted.
// Two categories:
//
//  1. TYPED HOOKS (HookManager::Add)
//     Use these when you need to inspect or modify arguments/return values.
//     Each one requires a Detour + Original pair in a Mods/ file.
//
//     Each entry follows the pattern:
//
//       Add(AbsFromRva(RVA_FUNCTION), Detour, &Original, "DisplayName");
//
//       AbsFromRva  — converts the compile-time RVA constant to a runtime address
//       Detour      — your replacement function defined in e.g. Randomizer.cpp
//       &Original   — trampoline pointer; call this inside the detour to call through
//       DisplayName — shown in debug output to identify which hook failed
//
//     Adding a new hook to the mod means adding one line here and defining the
//     detour/original pair in the relevant file under Mods/. Nothing else changes.
//
//  2. AUTO LOG HOOKS (CallLogger::RegisterAll)
//     Use these when you just want to know that a function was called.
//     Pass a map of { RVA → display name }. No extra code needed.
//     See CallLogger.h for the float-param caveat.
// ============================================================================

static void RegisterHooks() {

    // ------------------------------------------------------------------
    // 1. Typed hooks — mod logic lives in the corresponding Mods/ file
    // ------------------------------------------------------------------

    // --- Car system ---
    HookManager::Add(
        AbsFromRva(RVA_LOAD_VANILLA_CAR_POOL),
        reinterpret_cast<void*>(Randomizer::Hook_LoadVanillaCarPool),
        reinterpret_cast<void**>(&Randomizer::Orig_LoadVanillaCarPool),
        "LoadVanillaCarPool"
    );

    HookManager::Add(
        AbsFromRva(RVA_LOAD_CUSTOM_CAR_POOL),
        reinterpret_cast<void*>(Randomizer::Hook_LoadCustomCarPool),
        reinterpret_cast<void**>(&Randomizer::Orig_LoadCustomCarPool),
        "LoadCustomCarPool"
    );

    HookManager::Add(
        AbsFromRva(RVA_SYNC_CAR_INFO_FROM_PHYSICS),
        reinterpret_cast<void*>(Randomizer::Hook_SyncCarInfoFromPhysics),
        reinterpret_cast<void**>(&Randomizer::Orig_SyncCarInfoFromPhysics),
        "SyncCarInfoFromPhysics"
    );

    HookManager::Add(
        AbsFromRva(RVA_LOAD_TEXTURE_BY_NAME),
        reinterpret_cast<void*>(Randomizer::Hook_LoadTextureByName),
        reinterpret_cast<void**>(&Randomizer::Orig_LoadTextureByName),
        "LoadTextureByName"
    );

    HookManager::Add(
        AbsFromRva(RVA_LOAD_VANILLA_TRACKS),
        reinterpret_cast<void*>(Randomizer::Hook_LoadVanillaTracks),
        reinterpret_cast<void**>(&Randomizer::Orig_LoadVanillaTracks),
        "LoadVanillaTracks"
    );

    HookManager::Add(
        AbsFromRva(RVA_LOAD_CUSTOM_TRACKS),
        reinterpret_cast<void*>(Randomizer::Hook_LoadCustomTracks),
        reinterpret_cast<void**>(&Randomizer::Orig_LoadCustomTracks),
        "LoadCustomTracks"
    );

    HookManager::Add(
        AbsFromRva(RVA_LOAD_VANILLA_CUPS),
        reinterpret_cast<void*>(Randomizer::Hook_LoadVanillaCups),
        reinterpret_cast<void**>(&Randomizer::Orig_LoadVanillaCups),
        "LoadVanillaCups"
    );

    HookManager::Add(
        AbsFromRva(RVA_LOAD_CUSTOM_CUPS),
        reinterpret_cast<void*>(Randomizer::Hook_LoadCustomCups),
        reinterpret_cast<void**>(&Randomizer::Orig_LoadCustomCups),
        "LoadCustomCups"
    );

    HookManager::Add(
        AbsFromRva(RVA_CUP_VALIDATE_AND_CHECK_UNLOCK),
        reinterpret_cast<void*>(Randomizer::Hook_Cup_ValidateAndCheckUnlock),
        reinterpret_cast<void**>(&Randomizer::Orig_Cup_ValidateAndCheckUnlock),
        "Cup_ValidateAndCheckUnlock"
    );

    HookManager::Add(
        AbsFromRva(RVA_UPDATE_CAR_SELECTABILITY),
        reinterpret_cast<void*>(Randomizer::Hook_UpdateCarSelectability),
        reinterpret_cast<void**>(&Randomizer::Orig_UpdateCarSelectability),
        "UpdateCarSelectability"
    );

    HookManager::Add(
        AbsFromRva(RVA_GET_PROFILE_INDEX),
        reinterpret_cast<void*>(Randomizer::Hook_GetProfileIndex),
        reinterpret_cast<void**>(&Randomizer::Orig_GetProfileIndex),
        "GetProfileIndex"
    );

    HookManager::Add(
        AbsFromRva(RVA_PROFILE_CREATE_OR_LOAD),
        reinterpret_cast<void*>(Randomizer::Hook_Profile_CreateOrLoad),
        reinterpret_cast<void**>(&Randomizer::Orig_Profile_CreateOrLoad),
        "Profile_CreateOrLoad"
    );

    HookManager::Add(
        AbsFromRva(RVA_PROFILE_LOAD_AND_RESET),
        reinterpret_cast<void*>(Randomizer::Hook_Profile_LoadAndReset),
        reinterpret_cast<void**>(&Randomizer::Orig_Profile_LoadAndReset),
        "Profile_LoadAndReset"
    );

    HookManager::Add(
        AbsFromRva(RVA_LOAD_SETTINGS_FROM_INI),
        reinterpret_cast<void*>(Randomizer::Hook_LoadSettingsFromIni),
        reinterpret_cast<void**>(&Randomizer::Orig_LoadSettingsFromIni),
        "LoadSettingsFromIni"
    );

    HookManager::Add(
        AbsFromRva(RVA_TRACK_APPLY_CUSTOM_UNLOCK),
        reinterpret_cast<void*>(Randomizer::Hook_Track_ApplyCustomUnlock),
        reinterpret_cast<void**>(&Randomizer::Orig_Track_ApplyCustomUnlock),
        "Track_ApplyCustomUnlock"
    );

    HookManager::Add(
        AbsFromRva(RVA_CHECK_IF_TIER_CHAMPIONSHIP_WON),
        reinterpret_cast<void*>(Randomizer::Hook_CheckIfTierChampionshipWon),
        reinterpret_cast<void**>(&Randomizer::Orig_CheckIfTierChampionshipWon),
        "CheckIfTierChampionshipWon"
    );

    HookManager::Add(
        AbsFromRva(RVA_CHECK_IF_TIER_TIME_TRIALS_BEATEN),
        reinterpret_cast<void*>(Randomizer::Hook_CheckIfTierTimeTrialsBeaten),
        reinterpret_cast<void**>(&Randomizer::Orig_CheckIfTierTimeTrialsBeaten),
        "CheckIfTierTimeTrialsBeaten"
    );

    HookManager::Add(
        AbsFromRva(RVA_CHECK_IF_TIER_PRACTICE_STARS_FOUND),
        reinterpret_cast<void*>(Randomizer::Hook_CheckIfTierPracticeStarsFound),
        reinterpret_cast<void**>(&Randomizer::Orig_CheckIfTierPracticeStarsFound),
        "CheckIfTierPracticeStarsFound"
    );

    HookManager::Add(
        AbsFromRva(RVA_CHECK_IF_TIER_SINGLE_RACES_WON),
        reinterpret_cast<void*>(Randomizer::Hook_CheckIfTierSingleRacesWon),
        reinterpret_cast<void**>(&Randomizer::Orig_CheckIfTierSingleRacesWon),
        "CheckIfTierSingleRacesWon"
    );
    
    HookManager::Add(
        AbsFromRva(RVA_INIT_CAR_PHYSICS_BLOCK),
        reinterpret_cast<void*>(Randomizer::Hook_InitCarPhysicsBlock),
        reinterpret_cast<void**>(&Randomizer::Orig_InitCarPhysicsBlock),
        "InitCarPhysicsBlock"
    );

    HookManager::Add(
        AbsFromRva(RVA_TOKEN_MATCHES),
        reinterpret_cast<void*>(Randomizer::Hook_Token_Matches),
        reinterpret_cast<void**>(&Randomizer::Orig_Token_Matches),
        "Token_Matches"
    );

    HookManager::Add(
        AbsFromRva(RVA_READ_TOKEN_FLOAT),
        reinterpret_cast<void*>(Randomizer::Hook_ReadTokenFloat),
        reinterpret_cast<void**>(&Randomizer::Orig_ReadTokenFloat),
        "ReadTokenFloat"
    );

    HookManager::Add(
        AbsFromRva(RVA_READ_TOKEN_INT),
        reinterpret_cast<void*>(Randomizer::Hook_ReadTokenInt),
        reinterpret_cast<void**>(&Randomizer::Orig_ReadTokenInt),
        "ReadTokenInt"
    );

    HookManager::Add(
        AbsFromRva(RVA_READ_TOKEN_BOOL),
        reinterpret_cast<void*>(Randomizer::Hook_ReadTokenBool),
        reinterpret_cast<void**>(&Randomizer::Orig_ReadTokenBool),
        "ReadTokenBool"
    );

    HookManager::Add(
        AbsFromRva(RVA_DRAW_POST_RACE_LEADERBOARD),
        reinterpret_cast<void*>(Randomizer::Hook_DrawPostRaceLeaderboard),
        reinterpret_cast<void**>(&Randomizer::Orig_DrawPostRaceLeaderboard),
        "DrawPostRaceLeaderboard"
    );

    HookManager::Add(
        AbsFromRva(RVA_UPDATE_TIME_TRIAL_LEADERBOARDS),
        reinterpret_cast<void*>(Randomizer::Hook_UpdateTimeTrialLeaderboards),
        reinterpret_cast<void**>(&Randomizer::Orig_UpdateTimeTrialLeaderboards),
        "UpdateTimeTrialLeaderboards"
    );

    HookManager::Add(
        AbsFromRva(RVA_PICKUP_COLLECT_PROGRESS_OBJECT),
        reinterpret_cast<void*>(Randomizer::Hook_Pickup_CollectProgressObject),
        reinterpret_cast<void**>(&Randomizer::Orig_Pickup_CollectProgressObject),
        "Pickup_CollectProgressObject"
    );

    HookManager::Add(
        AbsFromRva(RVA_ENGINE_UPDATE_RACE_PROGRESS),
        reinterpret_cast<void*>(Randomizer::Hook_Engine_UpdateRaceProgress),
        reinterpret_cast<void**>(&Randomizer::Orig_Engine_UpdateRaceProgress),
        "Engine_UpdateRaceProgress"
    );

    HookManager::Add(
        AbsFromRva(RVA_CUP_ON_STAGE_FINISHED),
        reinterpret_cast<void*>(Randomizer::Hook_Cup_OnStageFinished),
        reinterpret_cast<void**>(&Randomizer::Orig_Cup_OnStageFinished),
        "Cup_OnStageFinished"
    );

    HookManager::Add(
        AbsFromRva(RVA_CUP_GENERATE_OPPONENT_GRID),
        reinterpret_cast<void*>(Randomizer::Hook_Cup_GenerateOpponentGrid),
        reinterpret_cast<void**>(&Randomizer::Orig_Cup_GenerateOpponentGrid),
        "Cup_GenerateOpponentGrid"
    );

    HookManager::Add(
        AbsFromRva(RVA_BUILD_GRID),
        reinterpret_cast<void*>(Randomizer::Hook_BuildGrid),
        reinterpret_cast<void**>(&Randomizer::Orig_BuildGrid),
        "BuildGrid"
    );

    HookManager::Add(
        AbsFromRva(RVA_UPDATE_CUP_POST_RACE_PROGRESS),
        reinterpret_cast<void*>(Randomizer::Hook_UpdateCupPostRaceProgress),
        reinterpret_cast<void**>(&Randomizer::Orig_UpdateCupPostRaceProgress),
        "UpdateCupPostRaceProgress"
    );

    HookManager::Add(
        AbsFromRva(RVA_DRAW_CUP_STANDINGS_TABLE),
        reinterpret_cast<void*>(Randomizer::Hook_DrawCupStandingsTable),
        reinterpret_cast<void**>(&Randomizer::Orig_DrawCupStandingsTable),
        "DrawCupStandingsTable"
    );

    HookManager::Add(
        AbsFromRva(RVA_DRAW_PROGRESS_TABLE),
        reinterpret_cast<void*>(Randomizer::Hook_DrawProgressTable),
        reinterpret_cast<void**>(&Randomizer::Orig_DrawProgressTable),
        "DrawProgressTable"
    );

    HookManager::Add(
        AbsFromRva(RVA_BUILD_START_RACE_MENU),
        reinterpret_cast<void*>(Randomizer::Hook_BuildStartRaceMenu),
        reinterpret_cast<void**>(&Randomizer::Orig_BuildStartRaceMenu),
        "BuildStartRaceMenu"
    );

    HookManager::Add(
        AbsFromRva(RVA_BUILD_OPTIONS_MENU),
        reinterpret_cast<void*>(Randomizer::Hook_BuildOptionsMenu),
        reinterpret_cast<void**>(&Randomizer::Orig_BuildOptionsMenu),
        "BuildOptionsMenu"
    );

    HookManager::Add(
        AbsFromRva(RVA_HANDLE_OPTIONS_MENU_ACTION),
        reinterpret_cast<void*>(Randomizer::Hook_HandleOptionsMenuAction),
        reinterpret_cast<void**>(&Randomizer::Orig_HandleOptionsMenuAction),
        "HandleOptionsMenuAction"
    );

    HookManager::Add(
        AbsFromRva(RVA_HANDLE_START_RACE_MENU_ACTION),
        reinterpret_cast<void*>(Randomizer::Hook_HandleStartRaceMenuAction),
        reinterpret_cast<void**>(&Randomizer::Orig_HandleStartRaceMenuAction),
        "HandleStartRaceMenuAction"
    );

    HookManager::Add(
        AbsFromRva(RVA_RACE_SESSION_SETUP),
        reinterpret_cast<void*>(Randomizer::Hook_RaceSessionSetup),
        reinterpret_cast<void**>(&Randomizer::Orig_RaceSessionSetup),
        "RaceSessionSetup"
    );

    HookManager::Add(
        AbsFromRva(RVA_SETUP_ALL_RACE_CARS),
        reinterpret_cast<void*>(Randomizer::Hook_SetupAllRaceCars),
        reinterpret_cast<void**>(&Randomizer::Orig_SetupAllRaceCars),
        "SetupAllRaceCars"
    );

    HookManager::Add(
        AbsFromRva(RVA_RANDOMIZE_CAR_PICKS),
        reinterpret_cast<void*>(Randomizer::Hook_RandomizeCarPicks),
        reinterpret_cast<void**>(&Randomizer::Orig_RandomizeCarPicks),
        "RandomizeCarPicks"
    );

    HookManager::Add(
        AbsFromRva(RVA_ASSIGN_START_POSITIONS),
        reinterpret_cast<void*>(Randomizer::Hook_AssignStartPositions),
        reinterpret_cast<void**>(&Randomizer::Orig_AssignStartPositions),
        "AssignStartPositions"
    );

    HookManager::Add(
        AbsFromRva(RVA_ADD_PARTICIPANT_AND_COUNT),
        reinterpret_cast<void*>(Randomizer::Hook_AddParticipantAndCount),
        reinterpret_cast<void**>(&Randomizer::Orig_AddParticipantAndCount),
        "AddParticipantAndCount"
    );

    HookManager::Add(
        AbsFromRva(RVA_UPDATE_RACE_POSITIONS),
        reinterpret_cast<void*>(Randomizer::Hook_UpdateRacePositions),
        reinterpret_cast<void**>(&Randomizer::Orig_UpdateRacePositions),
        "UpdateRacePositions"
    );

    HookManager::Add(
        AbsFromRva(RVA_LOAD_OBJECTS_FROM_FOB),
        reinterpret_cast<void*>(Randomizer::Hook_LoadObjectsFromFob),
        reinterpret_cast<void**>(&Randomizer::Orig_LoadObjectsFromFob),
        "LoadObjectsFromFob"
    );


    /* HookManager::Add(
        AbsFromRva(RVA_PARSE_PARAMETERS_TXT),
        reinterpret_cast<void*>(Randomizer::Hook_ParseParametersTxt),
        reinterpret_cast<void**>(&Randomizer::Orig_ParseParametersTxt),
        "ParseParametersTxt"
    ); */

    /* HookManager::Add(
        AbsFromRva(RVA_CREATE_CARBOX),
        reinterpret_cast<void*>(Randomizer::Hook_CreateCarbox),
        reinterpret_cast<void**>(&Randomizer::Orig_CreateCarbox),
        "CreateCarbox"
    );*/

    // Add further hooks here as the mod grows, e.g.:
    //
    // HookManager::Add(
    //     AbsFromRva(RVA_CALC_DELTA_TIME),
    //     reinterpret_cast<void*>(SlowMo::Hook_CalcDeltaTime),
    //     reinterpret_cast<void**>(&SlowMo::Orig_CalcDeltaTime),
    //     "CalcDeltaTime"
    // );

    // ------------------------------------------------------------------
    // 2. Auto log hooks — just a map, no extra code required
    //
    // Every function listed here will print its name to the log and the
    // VS Code Debug Console each time it is called by RVGL, then
    // transparently call the real function.
    //
    // RVAs from Addresses.h or the knowledge base (subtract image base
    // 0x00400000 from the Ghidra address to get the RVA).
    // ------------------------------------------------------------------

    #if defined(_DEBUG)
    CallLogger::RegisterAll({
        // Game loop
        // { 0x00001610, "FrameUpdateDispatcher"       },
        // { 0x0005a2b0, "InRaceGameLoop" },
        // { 0x001577f0, "FrontendGameLoop" },
        // { 0x00157400, "FUN_00557400_CallsCreateCarbox" },
        // { 0x00154870, "DrawProgressTable" }, // called for each frame the progress table is on the screen

        // Car system
        // { 0x0003F140, "LoadVanillaCarPool"          },   // same as typed hook above — pick one or the other
        // { 0x0003FAC0, "LoadCustomCarPool"           },
        // { 0x000F03A0, "CreateCarEntity"             },
        { 0x000F0630, "DestroyCarEntity"            },
        { 0x0004CB60, "InitPlayerCar"               },

        // Race session
        // { 0x00050700, "RaceSessionSetup"             },
        // { 0x0004F7B0, "SetupAllRaceCars"             },
        // { 0x0004FB30, "RandomizeCarPicks"            },
        // { 0x0004F980, "AssignStartPositions"         },
        // { 0x00041F80, "RaceEndManager"               }, // NOTE: check RVA
        // { 0x0008A560, "SetRaceState"                 },

        // Track / navigation
        // { 0x00052190, "LoadAllTracks"                },
        // { 0x00051AB0, "SetupRaceTrackRandom"         },
        // { 0x00040C30, "LevelLoad"                    },

        // Ghost / replay
        // { 0x0004CEC0, "RecordGhostFrame"             },
        // { 0x0004D040, "CommitGhostLapFrame"          },
        // { 0x0004D0C0, "RecordGhostPosition"          },

        // Carboxes
        { 0x001436a0, "LoadVanillaCarboxes" },
        { 0x001570d0, "CreateCarbox" },
        { 0x001599d0, "InitFrontendCarboxEntity" },
        
        //
        { 0x00143890, "Menu_InitializeFrontend" },
        { 0x00141180, "Gallery_InitDisplay" },
        { 0x001437d0, "Gallery_CheckShouldActivate" },
        { 0x0013f810, "Gallery_SetupCamera" },
        { 0x00141f80, "UnknownFnNotRaceEndManager" },
        // { 0x000753c0, "Profile_LoadAndReset" },
        // { 0x0004b800, "Cup_OnStageFinished" }, // typed hook installed above
        { 0x00006b40, "PrepareLevelLoad" },
        { 0x000f0890, "RegisterFinishTime" },
        { 0x00074bf0, "ResetProgressTable" },


        
    });
    #endif
}

// ============================================================================
// HookManager::InstallAll / RemoveAll
// ============================================================================

bool HookManager::InstallAll() {
    // Initialise MinHook once for the lifetime of the DLL.
    MH_STATUS status = MH_Initialize();

    if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED) {
        Logger::TimestampLogf("[HookManager] FAIL MH_Initialize — %s",
                     MH_StatusToString(status));
        return false;
    }

    g_minHookInitialized = true;
    RegisterHooks();

    // Report overall result but don't treat individual hook failures as fatal —
    // a partially-working mod is more useful than a mod that refuses to run.
    const size_t installed = g_hooks.size();
    Logger::TimestampLogf("[HookManager] InstallAll complete — %zu typed hook(s) active",
                 installed);

    return installed > 0;
}

void HookManager::RemoveAll() {
    if (!g_minHookInitialized)
        return;

    // Disable every typed hook we installed before uninitializing MinHook.
    for (const auto& entry : g_hooks) {
        if (entry.installed)
            MH_DisableHook(reinterpret_cast<void*>(entry.target));
    }

    g_hooks.clear();

    // Disable auto-log hooks
    CallLogger::UnregisterAll();
    
    MH_Uninitialize();
    g_minHookInitialized = false;

    Logger::TimestampLog("[HookManager] RemoveAll complete");
}
