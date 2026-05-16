#include "ArchipelagoEventKeys.h"

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

    return std::string(eventName) + ":" + name;
}

} // namespace ArchipelagoEventKeys
} // namespace Randomizer
