#include "RaceInitHooks.h"
#include "Logger.h"
#include "Addresses.h"
#include "RVGLStructs.h"
#include "30CarMod.h"
#include "ThirtyCarCupMod.h"

namespace Randomizer {

// ----------------------------------------------------------------------------
// Original function pointers
// MinHook writes the trampoline addresses into these during InstallAll().
// ----------------------------------------------------------------------------
FnDrawPostRaceLeaderboard Orig_DrawPostRaceLeaderboard = nullptr;
FnRaceSessionSetup        Orig_RaceSessionSetup        = nullptr;
FnAssignStartPositions    Orig_AssignStartPositions    = nullptr;
FnSetupAllRaceCars        Orig_SetupAllRaceCars        = nullptr;
FnRandomizeCarPicks       Orig_RandomizeCarPicks       = nullptr;
FnAddParticipantAndCount  Orig_AddParticipantAndCount  = nullptr;
FnUpdateRacePositions     Orig_UpdateRacePositions     = nullptr;

// ----------------------------------------------------------------------------
// Detour functions — registered in HookManager.cpp → RegisterHooks().
// ----------------------------------------------------------------------------
void Hook_DrawPostRaceLeaderboard() {

    // Logger::TimestampLogf("[RaceInitHooks] Calling DrawPostRaceLeaderboard");
    bool raceFinished = *reinterpret_cast<bool*>(AbsFromRva(RVA_RACE_FINISHED_FLAG));
    GameMode* gameMode = reinterpret_cast<GameMode*>(AbsFromRva(RVA_GAME_MODE));
    GameMode originalGameMode = *gameMode;
    int participantCount = GetParticipantCount();

    if (raceFinished && participantCount > 16) {
        *gameMode = MODE_CLOCKWORK_CARNAGE; // Override game mode to show race results in Clockwork Carnage mode
    }

    // Call the original function to draw the normal leaderboard.
    Orig_DrawPostRaceLeaderboard();

    if (raceFinished) {
        *gameMode = originalGameMode; // Restore the original game mode after drawing the leaderboard
    }

    // Logger::TimestampLogf("[RaceInitHooks] DrawPostRaceLeaderboard completed");
}

void Hook_RaceSessionSetup(bool isRestart) {
    Logger::TimestampLogf("[RaceInitHooks] Calling RaceSessionSetup with isRestart=%d", isRestart);

    // Logger::TimestampLogf("[RaceInitHooks] Overriding number of cars to 30");
    // int* nCars = reinterpret_cast<int*>(AbsFromRva(RVA_SETTINGS_NCARS));
    // *nCars = 30;
    ResetThirtyCarModState();
    ResetThirtyCarCupState();

    Orig_RaceSessionSetup(isRestart);
    Logger::TimestampLogf("[RaceInitHooks] RaceSessionSetup completed");
}

void Hook_AssignStartPositions() {
    Logger::TimestampLogf("[RaceInitHooks] Calling AssignStartPositions");
    ExpandRaceParticipantsToThirty();
    Orig_AssignStartPositions();
    Logger::TimestampLogf("[RaceInitHooks] AssignStartPositions completed");
}

void Hook_SetupAllRaceCars() {
    Logger::TimestampLogf("[RaceInitHooks] Calling SetupAllRaceCars");
    Orig_SetupAllRaceCars();

    ApplyThirtyCarGrid();
    ApplyThirtyCarCupGrid();

    Logger::TimestampLogf("[RaceInitHooks] SetupAllRaceCars completed");
}

void Hook_RandomizeCarPicks() {
    Logger::TimestampLogf("[RaceInitHooks] Calling RandomizeCarPicks");
    Orig_RandomizeCarPicks();
    Logger::TimestampLogf("[RaceInitHooks] RandomizeCarPicks completed");
}

bool Hook_AddParticipantAndCount(int carType, int spawnType, int carID, int skinID, int isLocal, int networkID, char *playerName) {
    Logger::TimestampLogf("[RaceInitHooks] Calling AddParticipantAndCount with carType=%d, spawnType=%d, carID=%d, skinID=%d, isLocal=%d, networkID=%d, playerName=%s",
                 carType, spawnType, carID, skinID, isLocal, networkID, playerName);
    bool result = Orig_AddParticipantAndCount(carType, spawnType, carID, skinID, isLocal, networkID, playerName);
    Logger::TimestampLogf("[RaceInitHooks] AddParticipantAndCount returned %d", result);
    return result;
}

void Hook_UpdateRacePositions() {
    Orig_UpdateRacePositions();
    MovePlayersToBackAfterRacePositions();
    MoveThirtyCarCupPlayerToBackAfterRacePositions();
}



} // namespace Randomizer
