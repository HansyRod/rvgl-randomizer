#include <windows.h>
#include "HookManager.h"
#include "Logger.h"
#include "RandomizerInit.h"

// ============================================================================
// DllMain
//
// Kept as thin as possible. All hook logic lives in HookManager.cpp.
// All mod logic lives in the individual mod files under Mods/.
// ============================================================================

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    switch (reason) {

    case DLL_PROCESS_ATTACH:

        #if defined(_DEBUG)
        MessageBoxA(NULL, "Attach Visual Studio now, then click OK!", "Waiting for Debugger...", MB_OK);
        #endif

        // We don't need thread attach/detach notifications.
        DisableThreadLibraryCalls(hModule);

        // A brief wait ensures the RVGL process has fully initialized its own
        // DLL dependencies before we start patching. In practice the suspended-
        // process injection approach means this is not strictly necessary, but
        // it costs nothing and prevents edge-case issues on slower machines.
        Sleep(50);

        Logger::Init(hModule);
        HookManager::InstallAll();
        Randomizer::Initialize();
        break;

    case DLL_PROCESS_DETACH:
        HookManager::RemoveAll();
        Logger::Shutdown();
        break;
    }

    return TRUE;
}
