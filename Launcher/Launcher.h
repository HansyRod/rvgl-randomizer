#pragma once
#include <cstdint>
#include <string>

// ============================================================================
// Launcher
//
// Starts RVGL in a suspended state, injects the mod DLL, then resumes.
// The UI calls Launch() when the user clicks the launch button.
// ============================================================================

struct LaunchConfig {
    std::string rvglExePath;   // Absolute path to rvgl.exe
    std::string modLibraryPath; // Absolute path to randomizer.dll / randomizer.so
    std::string extraArgs;     // Optional extra CLI args (e.g. "-window 1920 1080")
    std::string configPath;  // Optional path to randomized JSON
};

struct LaunchResult {
    bool        success;
    std::string errorMessage;  // Empty on success
    std::uint32_t processPid;  // 0 on failure
};

namespace Launcher {
    LaunchResult Launch(const LaunchConfig& config);
}