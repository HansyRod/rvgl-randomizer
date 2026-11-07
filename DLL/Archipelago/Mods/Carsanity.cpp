#include "Carsanity.h"
#include "ArchipelagoClient.h"
#include "ArchipelagoEventKeys.h"
#include "CarHooks.h"
#include "GameUtils.h"
#include "Logger.h"
#include "ProfileHooks.h"
#include "RandomizerState.h"
#include "RVGLMemory.h"
#include "RVGLMenuStructs.h"
#include <string>

namespace Randomizer {

FnCarSelectionHandler Orig_CarSelectionHandler = nullptr;

namespace {

// Native Pre-Race Summary labels: "Won Race:", "Yes", and "No".
constexpr int kWonRaceLabelId = 0x116;
constexpr int kYesLabelId = 0x41;
constexpr int kNoLabelId = 0x42;

bool g_raceResultProcessed = false;

int GetCarSelectionModelId() {
    RaceSettingsRuntime* settings = GetRaceSettings();
    const int playerSlot = GetActivePlayerSlot();
    if (settings == nullptr ||
        playerSlot < 0 ||
        playerSlot >= 4) {
        return -1;
    }
    return settings->playerModelIds[playerSlot];
}

std::string GetCarWinEventKey(const ArchipelagoConfig& config, const std::string& carFolder) {
    const std::string eventKey = ArchipelagoEventKeys::MakeNamedEventKey(
        ArchipelagoEventKeys::CarRaceWin, carFolder.c_str());
    if (eventKey.empty()) {
        return {};
    }
    // The event key must match an enabled Archipelago location.
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

void Hook_CarSelectionHandler(int slotIndex, int scrollY) {
    // Native drawing uses the car selected at entry, even if search/navigation
    // changes the selection during this frame. Capture the same car for our row.
    const int modelId = GetCarSelectionModelId();
    Orig_CarSelectionHandler(slotIndex, scrollY);

    const ConfigData* config = GetActiveConfig();
    // The extra row belongs only to profiles with Archipelago Carsanity enabled.
    if (config == nullptr ||
        !config->archipelago.has_value() ||
        !config->archipelago->carsanity) {
        return;
    }

    MenuPanelSlot* slot = GetMenuPanelSlot(slotIndex);
    // The same native callback also draws skin-selection arrows in other panels.
    if (slot == nullptr ||
        slot->panel != GetCarSelectionMenuPanel()) {
        return;
    }

    // Show race-win status only for regular Single Races, excluding Knockout.
    if (GetGameModeRuntime().mode != MODE_SINGLE_RACE ||
        (IsKnockoutModeEnabled() &&
         GetRandomizerContext().knockoutState.modeActive)) {
        return;
    }

    CarInfo* cars = GetCarInfoTable();
    // Validate the displayed model before looking up its configured car check.
    if (cars == nullptr ||
        modelId < 0 ||
        modelId >= GetRuntimeCarCount()) {
        return;
    }
    const std::string carFolder = GetCarFolderName(cars[modelId]);
    // Excluded cars have no Carsanity status to display.
    if (GetCarWinEventKey(*config->archipelago, carFolder).empty()) {
        return;
    }
    const auto* carWins = GetArchipelagoProfileCarWins(config->metadata.seed, config->archipelago->slotNumber);
    const bool won = carWins != nullptr && carWins->count(carFolder) != 0;

    // Read the current locale table so changing language also updates this row.
    char** localeStrings = GetLocaleStrings();
    if (localeStrings == nullptr) {
        return;
    }
    // Match the other car statistics labels without changing the shared locale string.
    std::string wonRaceLabel = localeStrings[kWonRaceLabelId];
    if (!wonRaceLabel.empty() && wonRaceLabel.back() == ':') {
        // Removes trailing colon from label
        wonRaceLabel.pop_back();
    }

    // Continue the native statistics columns below Transmission (row 7).
    // Row 8 still fits inside the native 188-pixel background, including its padding.
    const float y = slot->textY + static_cast<float>(scrollY + 8) * 16.0f;
    RVGL_DrawUIText(slot->textX, y, 8.0f, 12.0f, 0xffebebeb, wonRaceLabel.data(), 96.0f, 0);
    RVGL_DrawUIText(slot->textX + 112.0f, y, 8.0f, 12.0f, 0xffebebeb,
        localeStrings[won ? kYesLabelId : kNoLabelId], 112.0f, 0);
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
    // Use the car folder for the persistent checklist and event mapping.
    const std::string carFolder = GetCarFolderName(cars[modelId]);
    const std::string eventKey = GetCarWinEventKey(*config->archipelago, carFolder);
    // Cars without an enabled location mapping do not award a check.
    if (eventKey.empty()) {
        return;
    }
    // Save the car's win in the Archipelago profile so its Carsanity status survives restarts.
    RecordArchipelagoProfileCarWin(config->metadata.seed, config->archipelago->slotNumber, carFolder);
    QueueArchipelagoEventKey(eventKey);
    Logger::TimestampLogf("[Carsanity] Single Race won model=%d apKey='%s'", modelId, eventKey.c_str());
}

} // namespace Randomizer
