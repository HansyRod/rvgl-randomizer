#include "ProfileHooks.h"
#include "RandomizerState.h"
#include "Addresses.h"
#include "RVGLStructs.h"
#include "MenuMod.h"
#include "CarHooks.h"
#include "RVGLFunctions.h"
#include "Logger.h"
#include "ArchipelagoClient.h"
#include "ArchipelagoLog.h"
#include "ArchipelagoMenu.h"
#include "CustomUnlocks.h"
#include <rpc.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <limits>
#include <map>
#include <string>
#include <stdexcept>
#include <utility>

namespace Randomizer {

// ----------------------------------------------------------------------------
// Original function pointers
// MinHook writes the trampoline addresses into these during InstallAll().
// ----------------------------------------------------------------------------
FnGetProfileIndex        Orig_GetProfileIndex        = nullptr;
FnProfile_CreateOrLoad   Orig_Profile_CreateOrLoad   = nullptr;
FnProfile_LoadAndReset   Orig_Profile_LoadAndReset   = nullptr;
FnLoadSettingsFromIni    Orig_LoadSettingsFromIni    = nullptr;
FnIni_SaveProfile        Orig_Ini_SaveProfile        = nullptr;

bool skipNextProfileLoad = false;

namespace {

constexpr char kRandomizerProfileFileName[] = "rvgl-randomizer.ini";
constexpr char kRandomizerSection[] = "Randomizer";
constexpr char kKnockoutSection[] = "Knockout";

// These settings belong to the active RVGL profile, just like the other options
// in this file. Passwords remain in the connection menu/client and never enter JSON.
struct ArchipelagoProfileData {
    std::string uuid;
    std::string server;
    bool requiresPassword = true;
    // Seed -> slot number -> completed IDs awaiting server confirmation. Sending
    // a packet does not prove receipt; retain it until a server list confirms it.
    std::map<std::string, std::map<std::string, std::set<int64_t>>> completedLocations;
    // Seed -> slot number -> achieved goal locations, retained after confirmation.
    std::map<std::string, std::map<std::string, std::set<int64_t>>> completedGoalLocations;
    // Seed -> slot number -> owned item IDs. RVGL unlocks are boolean;
    // filler and repeated copies of an item have no additional unlock effect.
    std::map<std::string, std::map<std::string, std::set<int64_t>>> receivedItems;
    // Seed -> slot number -> car folders that have won a Single Race.
    std::map<std::string, std::map<std::string, std::set<std::string>>> carWins;
    // Runtime-only flag: retry failed saves on a later save or normal game exit.
    bool dirty = false;
};

ArchipelagoProfileData g_archipelagoProfileData;
std::string g_archipelagoProfilePath;

std::string Trim(const std::string& value) {
    const size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }

    const size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string GetProfileInternalName(const char* profileName) {
    if (profileName == nullptr) {
        return {};
    }

    // RVGL passes a 0x20-byte profile entry here. The internal profile name
    // occupies its first 0x10 bytes; using a bounded length also works for
    // callers that pass a regular null-terminated profile name.
    size_t length = 0;
    while (length < 0x10 && profileName[length] != '\0') {
        ++length;
    }
    return std::string(profileName, length);
}

std::string GetRandomizerProfilePath(const char* profileName) {
    const std::string internalName = GetProfileInternalName(profileName);
    if (internalName.empty()) {
        return {};
    }

    return "profiles/" + internalName + "/" + kRandomizerProfileFileName;
}

std::string ResolveRandomizerProfilePath(const std::string& virtualPath, bool isWriteMode) {
    char mutablePath[256]{};
    std::snprintf(mutablePath, sizeof(mutablePath), "%s", virtualPath.c_str());

    char resolvedPath[256]{};
    RVGL_VfsResolvePath(mutablePath, resolvedPath, '\0', isWriteMode, nullptr);
    if (resolvedPath[0] != '\0') {
        return resolvedPath;
    }

    return virtualPath;
}

bool IsValidUUID(const std::string& uuid) {
    return uuid.size() == 32 && std::all_of(uuid.begin(), uuid.end(),
        [](unsigned char c) { return std::isxdigit(c) != 0; });
}

bool SaveArchipelagoProfileData() {
    g_archipelagoProfileData.dirty = true;
    if (g_archipelagoProfilePath.empty() || !IsValidUUID(g_archipelagoProfileData.uuid)) {
        return false;
    }
    // A failed profile save must not truncate the UUID or previously saved progress.
    // Write alongside the old file, close it, then replace it only after a full write.
    const std::string temporaryPath = g_archipelagoProfilePath + ".tmp";
    const nlohmann::json data = {
        {"uuid", g_archipelagoProfileData.uuid}, {"server", g_archipelagoProfileData.server},
        {"requiresPassword", g_archipelagoProfileData.requiresPassword},
        {"completedLocations", g_archipelagoProfileData.completedLocations},
        {"completedGoalLocations", g_archipelagoProfileData.completedGoalLocations},
        {"receivedItems", g_archipelagoProfileData.receivedItems},
        {"carWins", g_archipelagoProfileData.carWins}
    };
    std::ofstream file(temporaryPath, std::ios::binary | std::ios::trunc);
    file << data.dump(2) << '\n';
    file.close();
    if (!file || !MoveFileExA(temporaryPath.c_str(), g_archipelagoProfilePath.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileA(temporaryPath.c_str());
        LogArchipelagoMessagef("Could not save Archipelago profile data: %s. Pending changes remain in memory.", g_archipelagoProfilePath.c_str());
        return false;
    }
    g_archipelagoProfileData.dirty = false;
    return true;
}

void RestoreArchipelagoProfileReceivedItems(const ConfigData& config) {
    const auto seedItems = g_archipelagoProfileData.receivedItems.find(config.metadata.seed);
    if (seedItems == g_archipelagoProfileData.receivedItems.end()) {
        return;
    }
    const auto slotItems = seedItems->second.find(std::to_string(config.archipelago->slotNumber));
    if (slotItems == seedItems->second.end()) {
        return;
    }
    // Restore logical ownership before connecting. The usual game-thread unlock
    // refresh applies it to RVGL's car, track and cup data once that data is ready.
    size_t restoredItems = 0;
    for (const auto& item : config.archipelago->items) {
        if (!item.first.empty() && slotItems->second.count(item.second) != 0) {
            MarkArchipelagoItemReceived(item.first);
            ++restoredItems;
        }
    }
    if (restoredItems != 0) {
        Logger::TimestampLogf("[Archipelago] Restored %zu cached item unlock(s).", restoredItems);
    }
}

void LoadArchipelagoProfileData(const char* profileName, const ConfigData& config) {
    // Load the startup profile once. RVGL can reload settings during menu flows;
    // retain our in-memory checks and connection instead of loading them again.
    if (!g_archipelagoProfilePath.empty()) {
        return;
    }
    const std::string internalName = GetProfileInternalName(profileName);
    if (internalName.empty()) {
        // RVGL loads global/default settings before selecting the startup profile.
        return;
    }
    const std::string virtualPath = "profiles/" + internalName + "/archipelago.json";
    const std::string resolvedPath = ResolveRandomizerProfilePath(virtualPath, true);
    g_archipelagoProfilePath = resolvedPath;
    try {
        if (std::filesystem::exists(resolvedPath)) {
            // Existing profile data: load its saved identity and connection preferences.
            std::ifstream file(resolvedPath);
            const nlohmann::json data = nlohmann::json::parse(file);
            g_archipelagoProfileData.uuid = data.at("uuid").get<std::string>();
            g_archipelagoProfileData.server = data.value("server", std::string{});
            g_archipelagoProfileData.requiresPassword = data.value("requiresPassword", true);
            // Read once with the profile, never from the per-frame network pump.
            // Files created before completion caching simply have no history yet.
            g_archipelagoProfileData.completedLocations = data.value("completedLocations",
                decltype(g_archipelagoProfileData.completedLocations){});
            g_archipelagoProfileData.completedGoalLocations = data.value("completedGoalLocations",
                decltype(g_archipelagoProfileData.completedGoalLocations){});
            g_archipelagoProfileData.receivedItems = data.value("receivedItems",
                decltype(g_archipelagoProfileData.receivedItems){});
            g_archipelagoProfileData.carWins = data.value("carWins",
                decltype(g_archipelagoProfileData.carWins){});
            if (!IsValidUUID(g_archipelagoProfileData.uuid)) {
                throw std::runtime_error("Invalid profile UUID.");
            }
        } else {
            // First use: create a new identity because this profile has no JSON file.
            UUID uuid;
            RPC_CSTR generatedUUID = nullptr;
            if (UuidCreate(&uuid) != RPC_S_OK || UuidToStringA(&uuid, &generatedUUID) != RPC_S_OK) {
                throw std::runtime_error("Could not generate a profile UUID.");
            }
            std::string& uuidString = g_archipelagoProfileData.uuid;
            uuidString = reinterpret_cast<const char*>(generatedUUID);
            RpcStringFreeA(&generatedUUID);
            uuidString.erase(std::remove(uuidString.begin(), uuidString.end(), '-'), uuidString.end());
            // Persist the UUID before connecting so the identity survives restarts.
            if (!SaveArchipelagoProfileData()) {
                return;
            }
        }
    } catch (const std::exception& error) {
        // Never regenerate an identity merely because its settings file is malformed.
        LogArchipelagoMessagef("Could not load profile data: %s File left unchanged.", error.what());
        g_archipelagoProfileData = {};
        return;
    }
    SetArchipelagoClientUUID(g_archipelagoProfileData.uuid);
    RestoreArchipelagoProfileReceivedItems(config);
    ResetArchipelagoMenuProfile(g_archipelagoProfileData.server);
    if (!g_archipelagoProfileData.server.empty() && !g_archipelagoProfileData.requiresPassword) {
        RequestArchipelagoConnection(g_archipelagoProfileData.server, "");
    }
}

int ClampCarsPerRace(int value) {
    return std::clamp(value, randomizerMinCarCount, randomizerMaxCarCount);
}

int ClampEliminationsAtOnce(int value) {
    return std::clamp(value, 1, randomizerMaxCarCount - 1);
}

int ClampEliminationFrequency(int value) {
    return std::clamp(value, 1, 10);
}

int ClampBinaryOption(int value) {
    return value == 0 ? 0 : 1;
}

bool ParseInteger(const std::string& text, int& value) {
    const std::string trimmed = Trim(text);
    if (trimmed.empty()) {
        return false;
    }

    char* end = nullptr;
    const long parsed = std::strtol(trimmed.c_str(), &end, 10);
    if (end == trimmed.c_str() || *end != '\0' ||
        parsed < (std::numeric_limits<int>::min)() ||
        parsed > (std::numeric_limits<int>::max)()) {
        return false;
    }

    value = static_cast<int>(parsed);
    return true;
}

void ResetCustomProfileOptions(int baseCarCount) {
    RandomizerContext& ctx = GetRandomizerContext();
    ctx.carState.carsPerRace = ClampCarsPerRace(baseCarCount);
    ctx.knockoutState.lapCountMode = 0;
    ctx.knockoutState.eliminationFrequencyLaps = 1;
    ctx.knockoutState.eliminationsPerEvent = 1;
    ctx.knockoutState.knockedOutGhostMode = 1;
}

bool SaveRandomizerProfileSettings(const char* profileName) {
    const std::string path = GetRandomizerProfilePath(profileName);
    if (path.empty()) {
        return true;
    }

    const RandomizerContext& ctx = GetRandomizerContext();
    const std::string resolvedPath = ResolveRandomizerProfilePath(path, true);
    std::ofstream file(resolvedPath, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        Logger::TimestampLogf(
            "[ProfileHooks] Could not save custom profile settings to %s",
            path.c_str()
        );
        return false;
    }

    file << ";==========================================================\n"
         << ";              RVGL Randomizer Settings\n"
         << ";==========================================================\n\n"
         << "[Randomizer]\n"
         << "CarsPerRace = " << ClampCarsPerRace(ctx.carState.carsPerRace) << "\n\n"
         << "[Knockout]\n"
         << "LapCountMode = " << ClampBinaryOption(ctx.knockoutState.lapCountMode) << "\n"
         << "EliminationFrequencyLaps = "
         << ClampEliminationFrequency(ctx.knockoutState.eliminationFrequencyLaps) << "\n"
         << "EliminationsPerEvent = "
         << ClampEliminationsAtOnce(ctx.knockoutState.eliminationsPerEvent) << "\n"
         << "KnockedOutGhostMode = "
         << ClampBinaryOption(ctx.knockoutState.knockedOutGhostMode) << "\n";

    if (!file.good()) {
        Logger::TimestampLogf(
            "[ProfileHooks] Could not save custom profile settings to %s",
            path.c_str()
        );
        return false;
    }

    return true;
}

void LoadRandomizerProfileSettings(char* profileName, int baseCarCount) {
    ResetCustomProfileOptions(baseCarCount);

    const std::string path = GetRandomizerProfilePath(profileName);
    if (path.empty()) {
        return;
    }

    const std::string resolvedPath = ResolveRandomizerProfilePath(path, false);
    std::ifstream file(resolvedPath);
    if (!file.is_open()) {
        SaveRandomizerProfileSettings(profileName);
        return;
    }

    RandomizerContext& ctx = GetRandomizerContext();
    std::string section;
    std::string line;
    while (std::getline(file, line)) {
        const size_t commentStart = line.find(';');
        if (commentStart != std::string::npos) {
            line.erase(commentStart);
        }

        line = Trim(line);
        if (line.empty()) {
            continue;
        }

        if (line.front() == '[' && line.back() == ']') {
            section = Trim(line.substr(1, line.size() - 2));
            continue;
        }

        const size_t separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }

        const std::string key = Trim(line.substr(0, separator));
        const std::string valueText = Trim(line.substr(separator + 1));
        int value = 0;
        if (!ParseInteger(valueText, value)) {
            continue;
        }

        if (section == kRandomizerSection && key == "CarsPerRace") {
            ctx.carState.carsPerRace = ClampCarsPerRace(value);
        }
        else if (section == kKnockoutSection && key == "LapCountMode") {
            ctx.knockoutState.lapCountMode = ClampBinaryOption(value);
        }
        else if (section == kKnockoutSection && key == "EliminationFrequencyLaps") {
            ctx.knockoutState.eliminationFrequencyLaps = ClampEliminationFrequency(value);
        }
        else if (section == kKnockoutSection && key == "EliminationsPerEvent") {
            ctx.knockoutState.eliminationsPerEvent = ClampEliminationsAtOnce(value);
        }
        else if (section == kKnockoutSection && key == "KnockedOutGhostMode") {
            ctx.knockoutState.knockedOutGhostMode = ClampBinaryOption(value);
        }
    }

}

} // anonymous namespace

void SaveArchipelagoProfileConnection(const std::string& server, bool requiresPassword) {
    g_archipelagoProfileData.server = server;
    g_archipelagoProfileData.requiresPassword = requiresPassword;
    SaveArchipelagoProfileData();
}

void MarkArchipelagoProfilePasswordRequired() {
    if (!g_archipelagoProfileData.requiresPassword) {
        SaveArchipelagoProfileConnection(g_archipelagoProfileData.server, true);
    }
}

void RecordArchipelagoProfileLocationCheck(const std::string& seed, int slot, int64_t locationId,
    bool goalLocation) {
    if (g_archipelagoProfilePath.empty() || !IsValidUUID(g_archipelagoProfileData.uuid)) {
        return;
    }
    auto& completedLocations = g_archipelagoProfileData.completedLocations[seed][std::to_string(slot)];
    const bool newCompletion = completedLocations.insert(locationId).second;
    const bool newGoalCompletion = goalLocation &&
        g_archipelagoProfileData.completedGoalLocations[seed][std::to_string(slot)].insert(locationId).second;
    if (newCompletion || newGoalCompletion || g_archipelagoProfileData.dirty) {
        // Save before submitting, even while connected. Normal exit is a retry
        // opportunity, not the only protection against losing an unsent check.
        SaveArchipelagoProfileData();
    }
}

std::set<int64_t> GetArchipelagoProfileLocationChecks(const std::string& seed, int slot) {
    const auto seedChecks = g_archipelagoProfileData.completedLocations.find(seed);
    if (seedChecks == g_archipelagoProfileData.completedLocations.end()) {
        return {};
    }
    const auto slotChecks = seedChecks->second.find(std::to_string(slot));
    return slotChecks == seedChecks->second.end() ? std::set<int64_t>{} : slotChecks->second;
}

void RecordArchipelagoProfileGoalLocations(const std::string& seed, int slot,
    const std::set<int64_t>& locationIds) {
    if (g_archipelagoProfilePath.empty() || !IsValidUUID(g_archipelagoProfileData.uuid) || locationIds.empty()) {
        return;
    }
    auto& completedGoalLocations = g_archipelagoProfileData.completedGoalLocations[seed][std::to_string(slot)];
    const size_t previousCount = completedGoalLocations.size();
    completedGoalLocations.insert(locationIds.begin(), locationIds.end());
    if (completedGoalLocations.size() != previousCount || g_archipelagoProfileData.dirty) {
        SaveArchipelagoProfileData();
    }
}

const std::set<int64_t>* GetArchipelagoProfileGoalLocations(const std::string& seed, int slot) {
    const auto seedLocations = g_archipelagoProfileData.completedGoalLocations.find(seed);
    if (seedLocations == g_archipelagoProfileData.completedGoalLocations.end()) {
        return nullptr;
    }
    const auto slotLocations = seedLocations->second.find(std::to_string(slot));
    return slotLocations == seedLocations->second.end() ? nullptr : &slotLocations->second;
}

void RemoveConfirmedArchipelagoProfileLocationChecks(
    const std::string& seed, int slot, const std::set<int64_t>& confirmedLocations) {
    const auto seedChecks = g_archipelagoProfileData.completedLocations.find(seed);
    if (seedChecks == g_archipelagoProfileData.completedLocations.end()) {
        return;
    }
    const auto slotChecks = seedChecks->second.find(std::to_string(slot));
    if (slotChecks == seedChecks->second.end()) {
        return;
    }
    bool changed = false;
    for (int64_t locationId : confirmedLocations) {
        if (slotChecks->second.erase(locationId) != 0) {
            changed = true;
        }
    }
    if (changed || g_archipelagoProfileData.dirty) {
        // Only server-confirmed checks may leave the recovery file. Save once
        // for the whole list; a failed write remains dirty for the usual retries.
        SaveArchipelagoProfileData();
    }
}

void RecordArchipelagoProfileCarWin(const std::string& seed, int slot, const std::string& carFolder) {
    if (g_archipelagoProfilePath.empty() || !IsValidUUID(g_archipelagoProfileData.uuid) ||
        seed.empty() || slot <= 0 || carFolder.empty()) {
        return;
    }
    auto& carWins = g_archipelagoProfileData.carWins[seed][std::to_string(slot)];
    const bool newWin = carWins.insert(carFolder).second;
    if (newWin || g_archipelagoProfileData.dirty) {
        // Retain wins in memory if saving fails; later saves and normal exit retry.
        SaveArchipelagoProfileData();
    }
}

const std::set<std::string>* GetArchipelagoProfileCarWins(const std::string& seed, int slot) {
    const auto& carWins = g_archipelagoProfileData.carWins;
    const auto seedWinsEntry = carWins.find(seed);
    if (seedWinsEntry == carWins.end()) {
        return nullptr;
    }
    const auto& seedWins = seedWinsEntry->second;
    const auto slotWins = seedWins.find(std::to_string(slot));
    if (slotWins == seedWins.end()) {
        return nullptr;
    }
    return &slotWins->second;
}

void SaveArchipelagoProfileReceivedItems(const std::string& seed, int slot) {
    if (g_archipelagoProfilePath.empty() || !IsValidUUID(g_archipelagoProfileData.uuid)) {
        return;
    }
    const ConfigData* config = GetActiveConfig();
    if (config == nullptr || !config->archipelago.has_value()) {
        return;
    }
    std::set<int64_t> receivedItems;
    {
        ArchipelagoRuntimeState& state = GetRandomizerContext().archipelagoState;
        std::lock_guard<std::mutex> lock(state.itemMutex);
        // Runtime unlock checks use names; persist their numerical IDs from the
        // same metadata mapping used to decode incoming Archipelago items.
        for (const auto& item : config->archipelago->items) {
            if (state.receivedItems.count(item.first) != 0) {
                receivedItems.insert(item.second);
            }
        }
    }
    auto& cachedItems = g_archipelagoProfileData.receivedItems[seed][std::to_string(slot)];
    if (cachedItems != receivedItems || g_archipelagoProfileData.dirty) {
        // Snapshot the whole applied inventory once per packet, after a full
        // server inventory has replaced it or incremental deliveries were added.
        // Release the inventory mutex before writing; failed saves use the same
        // dirty flag and shutdown retry as completed location checks.
        cachedItems = std::move(receivedItems);
        SaveArchipelagoProfileData();
    }
}

void FlushArchipelagoProfileData() {
    // Normal game-loop exit retries any failed save; clean data needs no write.
    if (g_archipelagoProfileData.dirty) {
        SaveArchipelagoProfileData();
    }
}

int Hook_GetProfileIndex(char* profileName) {
    int index = Orig_GetProfileIndex(profileName);
    if (index == -1) {
        Orig_Profile_CreateOrLoad(profileName);
        index = Orig_GetProfileIndex(profileName);
        
        // Profile_CreateOrLoad already calls Profile_LoadAndReset,
        // so we can skip it the next time it's called
        skipNextProfileLoad = true;
    }
    return index;
}

bool Hook_Profile_CreateOrLoad(char* displayName) {
    return Orig_Profile_CreateOrLoad(displayName);
}

void Hook_Profile_LoadAndReset(char* profileName) {
    // Profile already loaded in GetProfileIndex
    // This happens when we are loading a profile created
    // for the randomizer for the first time
    if (skipNextProfileLoad) {
        skipNextProfileLoad = false;
        return;
    }

    // Reset unlock check when loading a new profile
    // Otherwise we will get the popup message when switching profiles
    int carCount = GetRuntimeCarCount();
    CarRuntimeState& carState = GetRandomizerContext().carState;
    carState.checkCarUnlocksPopup = false;
    carState.carSelectableState.assign(carCount, false);

    Orig_Profile_LoadAndReset(profileName);

}

void Hook_LoadSettingsFromIni(char* profileName) {

    RandomizerContext& ctx = GetRandomizerContext();
    bool useCupDC = ctx.config.useCupDC;

    Orig_LoadSettingsFromIni(profileName);

    // Set CupDC flag according to the config info.
    bool* cupDCFlag = reinterpret_cast<bool*>(AbsFromRva(RVA_CUP_DC));
    *cupDCFlag = useCupDC;

    // Sync our cars per race count with the stored cars per race
    int nCars = *reinterpret_cast<int*>(AbsFromRva(RVA_SETTINGS_NCARS));
    LoadRandomizerProfileSettings(profileName, nCars);

    const ConfigData* config = GetActiveConfig();
    if (config != nullptr && config->archipelago.has_value()) {
        LoadArchipelagoProfileData(profileName, *config);
    }

    // Change the NCars settings row so it uses our functions
    PatchCarCountMenuDescriptor();

}

bool Hook_Ini_SaveProfile(char* profileName) {
    const bool nativeResult = Orig_Ini_SaveProfile(profileName);
    if (profileName == nullptr) {
        return nativeResult;
    }

    const bool customResult = SaveRandomizerProfileSettings(profileName);
    return nativeResult && customResult;
}

} // namespace Randomizer
