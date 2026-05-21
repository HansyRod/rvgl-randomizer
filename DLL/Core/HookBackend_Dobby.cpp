#include "HookBackend.h"

#if defined(__linux__)

#include "dobby.h"

namespace HookBackend {

bool Initialize() {
    return true;
}

bool CreateHook(void* target, void* detour, void** original, const char** error) {
    const int result = DobbyHook(target, detour, original);
    if (result == 0) {
        return true;
    }

    if (error != nullptr) {
        *error = "DobbyHook failed";
    }
    return false;
}

bool EnableHook(void*, const char**) {
    return true;
}

bool DisableHook(void* target, const char** error) {
    const int result = DobbyDestroy(target);
    if (result == 0) {
        return true;
    }

    if (error != nullptr) {
        *error = "DobbyDestroy failed";
    }
    return false;
}

void Uninitialize() {
}

} // namespace HookBackend

#endif
