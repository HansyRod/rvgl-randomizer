#include "Launcher.h"

#include <filesystem>
#include <iostream>
#include <string>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

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
#if defined(_WIN32)
    char buf[MAX_PATH]{};
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    std::filesystem::path path(buf);
#else
    char buf[4096]{};
    const ssize_t length = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (length <= 0) {
        return ".";
    }
    buf[length] = '\0';
    std::filesystem::path path(buf);
#endif

    return path.parent_path().string();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: TestLauncher <path/to/rvgl> [extra args] [config path]\n";
        return 1;
    }

    LaunchConfig config;
    config.rvglExePath = argv[1];
#if defined(_WIN32)
    config.modLibraryPath = (std::filesystem::path(GetExecutableDir()) / "randomizer.dll").string();
#else
    config.modLibraryPath = (std::filesystem::path(GetExecutableDir()) / "randomizer.so").string();
#endif
    config.extraArgs = (argc >= 3) ? argv[2] : "";
    config.configPath = (argc >= 4) ? argv[3] : "";

    std::cout << "[TestLauncher] RVGL : " << config.rvglExePath << "\n";
    std::cout << "[TestLauncher] Mod  : " << config.modLibraryPath << "\n";
    if (!config.extraArgs.empty()) {
      std::cout << "[TestLauncher] Args : " << config.extraArgs << "\n";
    }
    if (!config.configPath.empty()) {
      std::cout << "[TestLauncher] Config : " << config.configPath << "\n";
    }

    const LaunchResult result = Launcher::Launch(config);

    if (!result.success) {
        std::cerr << "[TestLauncher] FAILED: " << result.errorMessage << "\n";
        return 1;
    }

    std::cout << "[TestLauncher] OK — RVGL running (PID " << result.processPid << ")\n";
    return 0;
}
