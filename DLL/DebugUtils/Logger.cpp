#include "Logger.h"
#include <windows.h>
#include <cstdio>
#include <cstdarg>
#include <mutex>
#include <string>

namespace Logger {

namespace {
FILE*      g_file  = nullptr;
std::mutex g_mutex;
} // anonymous namespace

void Init() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_file) return;

    // Relative path — resolves to the process working directory.
    // When launched via VS Code cppvsdbg with "cwd": "${workspaceFolder}",
    // this lands in the project root next to your source files.
    g_file = fopen("rvgl-randomizer.log", "w");

    const char* msg = "[Logger] Logging to rvgl-randomizer.log\n";
    OutputDebugStringA(msg);
    if (g_file) {
        fputs(msg, g_file);
        fflush(g_file);
    }
}

void Shutdown() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_file) {
        fclose(g_file);
        g_file = nullptr;
    }
}

void Log(const char* message) {
    if (!message) return;
    OutputDebugStringA(message);
    OutputDebugStringA("\n");

    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_file) {
        fputs(message, g_file);
        fputc('\n', g_file);
        fflush(g_file);
    }
}

void Log(const std::string& message) {
    Log(message.c_str());
}

void Logf(const char* fmt, ...) {
    char buf[512]{};
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf) - 1, fmt, args);
    va_end(args);
    Log(buf);
}

void TimestampLog(const char* message) {
    if (!message) return;

    // SYSTEMTIME is all WORD (uint16) fields — no floats, XMM registers untouched.
    SYSTEMTIME t;
    GetLocalTime(&t);

    char buf[640]{};
    snprintf(buf, sizeof(buf) - 1,
             "[%04d-%02d-%02d %02d:%02d:%02d.%03d] %s",
             t.wYear, t.wMonth, t.wDay,
             t.wHour, t.wMinute, t.wSecond, t.wMilliseconds,
             message);

    Log(buf);
}

void TimestampLogf(const char* fmt, ...) {
    char buf[512]{};
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf) - 1, fmt, args);
    va_end(args);
    TimestampLog(buf);
}

} // namespace Logger