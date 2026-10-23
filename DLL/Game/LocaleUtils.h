#pragma once

namespace Randomizer {

// A native locale entry temporarily borrowed by a custom menu. Keep the original
// pointer tied to its table so changing language cannot restore a stale pointer.
struct LocaleStringOverride {
    int labelId;
    const char* replacement;
    char* original = nullptr;
    char** capturedLocaleStrings = nullptr;
    char** overriddenLocaleStrings = nullptr;
    bool originalCaptured = false;
};

bool OverrideLocaleString(LocaleStringOverride& stringOverride);
void RestoreLocaleString(LocaleStringOverride& stringOverride);

} // namespace Randomizer
