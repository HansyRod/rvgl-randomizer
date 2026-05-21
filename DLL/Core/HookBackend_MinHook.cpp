#include "HookBackend.h"

#if defined(_WIN32)

#include "MinHook.h"

namespace HookBackend {

namespace {
const char* StatusString(MH_STATUS status) {
    return MH_StatusToString(status);
}
} // anonymous namespace

bool Initialize() {
    const MH_STATUS status = MH_Initialize();
    return status == MH_OK || status == MH_ERROR_ALREADY_INITIALIZED;
}

bool CreateHook(void* target, void* detour, void** original, const char** error) {
    const MH_STATUS status = MH_CreateHook(target, detour, original);
    if (status == MH_OK || status == MH_ERROR_ALREADY_CREATED) {
        return true;
    }

    if (error != nullptr) {
        *error = StatusString(status);
    }
    return false;
}

bool EnableHook(void* target, const char** error) {
    const MH_STATUS status = MH_EnableHook(target);
    if (status == MH_OK || status == MH_ERROR_ENABLED) {
        return true;
    }

    if (error != nullptr) {
        *error = StatusString(status);
    }
    return false;
}

bool DisableHook(void* target, const char** error) {
    const MH_STATUS status = MH_DisableHook(target);
    if (status == MH_OK || status == MH_ERROR_DISABLED) {
        return true;
    }

    if (error != nullptr) {
        *error = StatusString(status);
    }
    return false;
}

void Uninitialize() {
    MH_Uninitialize();
}

} // namespace HookBackend

#endif
