#include "ProfileHooks.h"
#include "RandomizerState.h"
#include "Addresses.h"
#include "RVGLStructs.h"
#include "MenuMod.h"
#include "CarHooks.h"
#include "RVGLFunctions.h"
#include "Logger.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <string>

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
