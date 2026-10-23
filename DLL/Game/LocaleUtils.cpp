#include "LocaleUtils.h"
#include "RVGLMemory.h"

namespace Randomizer {
namespace {

bool SaveOriginalLocaleString(LocaleStringOverride& stringOverride) {
    char** localeStrings = GetLocaleStrings();
    if (localeStrings == nullptr) {
        return false;
    }

    if (!stringOverride.originalCaptured ||
        stringOverride.capturedLocaleStrings != localeStrings) {
        stringOverride.original = localeStrings[stringOverride.labelId];
        stringOverride.capturedLocaleStrings = localeStrings;
        stringOverride.originalCaptured = true;
    }

    return true;
}

} // anonymous namespace

bool OverrideLocaleString(LocaleStringOverride& stringOverride) {
    if (!SaveOriginalLocaleString(stringOverride)) {
        return false;
    }

    char** localeStrings = GetLocaleStrings();
    // RVGL's table uses char*, but custom menu labels are read-only string literals.
    localeStrings[stringOverride.labelId] = const_cast<char*>(
        stringOverride.replacement != nullptr ? stringOverride.replacement : "");
    stringOverride.overriddenLocaleStrings = localeStrings;
    return true;
}

void RestoreLocaleString(LocaleStringOverride& stringOverride) {
    if (!stringOverride.originalCaptured ||
        stringOverride.overriddenLocaleStrings == nullptr) {
        return;
    }

    if (GetLocaleStrings() == stringOverride.overriddenLocaleStrings) {
        stringOverride.overriddenLocaleStrings[stringOverride.labelId] =
            stringOverride.original;
    }

    stringOverride.overriddenLocaleStrings = nullptr;
}

} // namespace Randomizer
