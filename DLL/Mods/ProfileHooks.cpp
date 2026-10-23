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
#include <rpc.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <limits>
#include <string>
#include <stdexcept>

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
    if (g_archipelagoProfilePath.empty() || !IsValidUUID(g_archipelagoProfileData.uuid)) {
        return false;
    }
    // A failed server-settings save must not truncate the UUID we need to reconnect.
    // Write alongside the old file, close it, then replace it only after a full write.
    const std::string temporaryPath = g_archipelagoProfilePath + ".tmp";
    const nlohmann::json data = {
        {"uuid", g_archipelagoProfileData.uuid}, {"server", g_archipelagoProfileData.server},
        {"requiresPassword", g_archipelagoProfileData.requiresPassword}
    };
    std::ofstream file(temporaryPath, std::ios::binary | std::ios::trunc);
    file << data.dump(2) << '\n';
    file.close();
    if (!file || !MoveFileExA(temporaryPath.c_str(), g_archipelagoProfilePath.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileA(temporaryPath.c_str());
        LogArchipelagoMessage("[Archipelago] Could not save Archipelago profile data.");
        return false;
    }
    return true;
}

void LoadArchipelagoProfileData(const char* profileName) {
    const std::string internalName = GetProfileInternalName(profileName);
    if (internalName.empty()) {
        // RVGL loads global/default settings before selecting the startup profile.
        return;
    }
    const std::string virtualPath = "profiles/" + internalName + "/archipelago.json";
    const std::string resolvedPath = ResolveRandomizerProfilePath(virtualPath, true);
    if (g_archipelagoProfilePath == resolvedPath) {
        return;
    }
    // Settings are reloaded by other frontend flows too. Only the first actual
    // profile in this process may auto-connect; switching profiles must remain manual.
    const bool firstProfile = g_archipelagoProfilePath.empty();
    CancelArchipelagoConnection();
    // If reading the new profile fails, Connect must not reuse the old profile's UUID.
    SetArchipelagoClientUUID("");
    ResetArchipelagoMenuProfile("");
    g_archipelagoProfilePath = resolvedPath;
    g_archipelagoProfileData = {};
    try {
        if (std::filesystem::exists(resolvedPath)) {
            // Existing profile data: load its saved identity and connection preferences.
            std::ifstream file(resolvedPath);
            const nlohmann::json data = nlohmann::json::parse(file);
            g_archipelagoProfileData.uuid = data.at("uuid").get<std::string>();
            g_archipelagoProfileData.server = data.value("server", std::string{});
            g_archipelagoProfileData.requiresPassword = data.value("requiresPassword", true);
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
        LogArchipelagoMessagef("[Archipelago] Could not load profile data: %s File left unchanged.", error.what());
        g_archipelagoProfileData = {};
        return;
    }
    SetArchipelagoClientUUID(g_archipelagoProfileData.uuid);
    ResetArchipelagoMenuProfile(g_archipelagoProfileData.server);
    if (firstProfile && !g_archipelagoProfileData.server.empty() && !g_archipelagoProfileData.requiresPassword) {
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
        LoadArchipelagoProfileData(profileName);
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
