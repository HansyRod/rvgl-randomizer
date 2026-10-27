#include "ArchipelagoEventKeys.h"
#include <algorithm>
#include <cctype>

namespace Randomizer {
namespace ArchipelagoEventKeys {
namespace {

constexpr int kMaxInitialStuntStarCheckNumber = 20;

} // anonymous namespace

int GetStuntStarCheckNumber(int starId) {
    const int checkNumber = starId + 1;
    return checkNumber >= 1 && checkNumber <= kMaxInitialStuntStarCheckNumber
        ? checkNumber
        : -1;
}

std::string MakeIndexedEventKey(const char* eventName, int checkNumber) {
    if (eventName == nullptr || eventName[0] == '\0' || checkNumber <= 0) {
        return "";
    }

    return std::string(eventName) + ":" + std::to_string(checkNumber);
}

std::string MakeNamedEventKey(const char* eventName, const char* name) {
    if (eventName == nullptr || eventName[0] == '\0' || name == nullptr || name[0] == '\0') {
        return "";
    }

    std::string eventKey = std::string(eventName) + ":" + name;
    // Named keys use lowercase to match the generated Archipelago mappings.
    std::transform(eventKey.begin(), eventKey.end(), eventKey.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return eventKey;
}

} // namespace ArchipelagoEventKeys
} // namespace Randomizer
