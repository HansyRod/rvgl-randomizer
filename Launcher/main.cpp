#include "Launcher.h"
#include <iostream>
#include <string>
#include <windows.h>

// ============================================================================
// TestLauncher entry point
//
// Minimal console wrapper around Launcher::Launch for development testing.
// In production, Launcher::Launch is called directly from the UI.
//
// Usage:
//   TestLauncher.exe "C:\path\to\rvgl.exe"
//   TestLauncher.exe "C:\path\to\rvgl.exe" "-window 1920 1080"
// ============================================================================

static std::string GetExecutableDir() {
    char buf[MAX_PATH]{};
    GetModuleFileNameA(nullptr, buf, MAX_PATH);

    std::string path(buf);
    const size_t slash = path.find_last_of("\\/");
    return (slash != std::string::npos) ? path.substr(0, slash) : ".";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: TestLauncher.exe <path\\to\\rvgl.exe> [extra args]\n";
        return 1;
    }

    LaunchConfig config;
    config.rvglExePath = argv[1];
    config.modDllPath  = GetExecutableDir() + "\\randomizer.dll";
    config.extraArgs   = (argc >= 3) ? argv[2] : "";

    std::cout << "[TestLauncher] RVGL : " << config.rvglExePath << "\n";
    std::cout << "[TestLauncher] DLL  : " << config.modDllPath  << "\n";
    if (!config.extraArgs.empty())
        std::cout << "[TestLauncher] Args : " << config.extraArgs << "\n";

    const LaunchResult result = Launcher::Launch(config);

    if (!result.success) {
        std::cerr << "[TestLauncher] FAILED: " << result.errorMessage << "\n";
        return 1;
    }

    std::cout << "[TestLauncher] OK — RVGL running (PID " << result.processPid << ")\n";
    return 0;
}
