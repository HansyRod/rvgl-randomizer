#include "HookManager.h"
#include "Logger.h"
#include "Platform.h"
#include "RandomizerInit.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif

namespace {

void OnLibraryLoad(Platform::ModuleHandle module) {
#if defined(_WIN32) && defined(_DEBUG)
    MessageBoxA(nullptr, "Attach Visual Studio now, then click OK!", "Waiting for Debugger...", MB_OK);
#endif

#if defined(_WIN32)
    // A brief wait ensures the RVGL process has fully initialized its own
    // DLL dependencies before we start patching. In practice the suspended-
    // process injection approach means this is not strictly necessary, but
    // it costs nothing and prevents edge-case issues on slower machines.
    Sleep(50);
#endif

    Logger::Init(module);
    HookManager::InstallAll();
    Randomizer::Initialize();
}

void OnLibraryUnload() {
    HookManager::RemoveAll();
    Logger::Shutdown();
}

} // anonymous namespace

#if defined(_WIN32)

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
        OnLibraryLoad(hModule);
        break;

    case DLL_PROCESS_DETACH:
        OnLibraryUnload();
        break;
    }

    return TRUE;
}

#else

__attribute__((constructor))
static void RandomizerSharedObjectAttach() {
    Dl_info info{};
    Platform::ModuleHandle module = nullptr;

    if (dladdr(reinterpret_cast<void*>(&RandomizerSharedObjectAttach), &info) != 0) {
        module = info.dli_fbase;
    }

    OnLibraryLoad(module);
}

__attribute__((destructor))
static void RandomizerSharedObjectDetach() {
    OnLibraryUnload();
}

#endif
