#include "Carsanity.h"
#include "ArchipelagoClient.h"
#include "ArchipelagoEventKeys.h"
#include "CarHooks.h"
#include "GameUtils.h"
#include "Logger.h"
#include "ProfileHooks.h"
#include "RandomizerState.h"
#include "RVGLMemory.h"
#include <string>

namespace Randomizer {
namespace {

bool g_raceResultProcessed = false;

std::string GetCarWinEventKey(const ArchipelagoConfig& config, const CarInfo& car) {
    const std::string carFolder = GetCarFolderName(car);
    const std::string eventKey = ArchipelagoEventKeys::MakeNamedEventKey(
        ArchipelagoEventKeys::CarRaceWin, carFolder.c_str());
    if (eventKey.empty()) {
        return {};
    }
    // The normalized event key must match an enabled Archipelago location.
    const auto location = config.locations.find(eventKey);
    if (location == config.locations.end() ||
        location->second <= 0) {
        return {};
    }
    return eventKey;
}

} // anonymous namespace

void ResetCarsanityRaceState() {
    g_raceResultProcessed = false;
}

void UpdateCarsanityRaceProgress() {
    const ConfigData* config = GetActiveConfig();
    // Detect car wins only when the active configuration enables Archipelago Carsanity.
    if (config == nullptr ||
        !config->archipelago.has_value() ||
        !config->archipelago->carsanity) {
        return;
    }

    // Only regular Single Races count; Knockout also uses the native Single Race mode.
    if (GetGameModeRuntime().mode != MODE_SINGLE_RACE ||
        (IsKnockoutModeEnabled() &&
         GetRandomizerContext().knockoutState.modeActive)) {
        return;
    }

    // Handle each race result once, until a new race or restart resets this flag.
    if (g_raceResultProcessed) {
        return;
    }

    CarEntityRuntime* player = GetLocalPlayerCar();
    RaceResultEntry* results = GetRaceResultTable();
    // Wait until the race has finished and the local player's finish data is available.
    if (!IsRaceFinished() ||
        player == nullptr ||
        player->finishTimeMs <= 0 ||
        results == nullptr) {
        return;
    }
    g_raceResultProcessed = true;

    // A finished race counts only if the local player won without a DNF.
    // The first result entry must match both the player and their recorded finish time.
    if (player->finishPosition != 0 ||
        results[0].car != player ||
        results[0].dnfFlag != 0 ||
        results[0].totalTimeMs != static_cast<uint32_t>(player->finishTimeMs)) {
        return;
    }

    const int modelId = player->transform.modelId;
    CarInfo* cars = GetCarInfoTable();
    // Validate the live car's model index before looking up its Carsanity check.
    if (cars == nullptr ||
        modelId < 0 ||
        modelId >= GetRuntimeCarCount()) {
        return;
    }
    const std::string eventKey = GetCarWinEventKey(*config->archipelago, cars[modelId]);
    // Cars without an enabled location mapping do not award a check.
    if (eventKey.empty()) {
        return;
    }
    // Use the normalized folder from the event key for the persistent checklist.
    const std::string carFolder = eventKey.substr(eventKey.find(':') + 1);
    // Save the car's win in the Archipelago profile so its Carsanity status survives restarts.
    RecordArchipelagoProfileCarWin(config->metadata.seed, config->archipelago->slotNumber, carFolder);
    QueueArchipelagoEventKey(eventKey);
    Logger::TimestampLogf("[Carsanity] Single Race won model=%d apKey='%s'", modelId, eventKey.c_str());
}

} // namespace Randomizer
