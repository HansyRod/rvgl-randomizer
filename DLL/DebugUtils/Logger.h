#pragma once
#include "Platform.h"
#include <string>

// ============================================================================
// Logger
//
// Writes messages to two sinks simultaneously:
//   1. OutputDebugStringA  — visible in the VS Code Debug Console when
//                            the process is launched with cppvsdbg.
//   2. A log file          — written next to the randomizer app bundle so it
//                            survives after the process exits and stays out of
//                            the RVGL install directory.
//
// Usage:
//   Logger::Init(module);     // call once from platform library entry point
//   Logger::Log("hi");        // plain message
//   Logger::Logf("car %d", n); // printf-style
//   Logger::TimestampLog("hi");        // plain message with timestamp
//   Logger::TimestampLogf("car %d", n); // printf-style with timestamp
//   Logger::Shutdown();       // call from DllMain detach / HookManager::RemoveAll
// ============================================================================

namespace Logger {

    // Opens the log file. Safe to call multiple times (no-op after first call).
    void Init(Platform::ModuleHandle selfModule);

    // Flushes and closes the log file.
    void Shutdown();

    // Write one line. A newline is always appended.
    void Log(const char* message);
    void Log(const std::string& message);

    // printf-style convenience wrapper.
    void Logf(const char* fmt, ...);

    // Prepends a full datetime prefix (YYYY-MM-DD HH:MM:SS.mmm) then calls Log.
    // Safe to call from inside a hook — no floating-point ops.
    void TimestampLog(const char* message);
    void TimestampLogf(const char* fmt, ...);

} // namespace Logger
