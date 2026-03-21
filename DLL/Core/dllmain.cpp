#include <windows.h>
#include "HookManager.h"

// ============================================================================
// DllMain
//
// Kept as thin as possible. All hook logic lives in HookManager.cpp.
// All mod logic lives in the individual mod files under Mods/.
// ============================================================================

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    switch (reason) {

    case DLL_PROCESS_ATTACH:
        // We don't need thread attach/detach notifications.
        DisableThreadLibraryCalls(hModule);

        // A brief wait ensures the RVGL process has fully initialized its own
        // DLL dependencies before we start patching. In practice the suspended-
        // process injection approach means this is not strictly necessary, but
        // it costs nothing and prevents edge-case issues on slower machines.
        Sleep(50);

        HookManager::InstallAll();
        break;

    case DLL_PROCESS_DETACH:
        HookManager::RemoveAll();
        break;
    }

    return TRUE;
}
