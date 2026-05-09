#include "Logger.h"
#include <windows.h>
#include <filesystem>
#include <cstdio>
#include <cstdarg>
#include <system_error>
#include <mutex>
#include <string>

namespace Logger {

namespace {
FILE*       g_file  = nullptr;
std::mutex  g_mutex;
std::string g_logPath;

std::string WideToUtf8(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }

    const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);

    if (size <= 0) {
        return {};
    }

    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), size, nullptr, nullptr);
    return result;
}

std::filesystem::path ResolveAppRoot(HMODULE selfModule) {
    wchar_t modulePath[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(selfModule, modulePath, MAX_PATH);

    if (length == 0 || length >= MAX_PATH) {
        return std::filesystem::current_path();
    }

    std::filesystem::path dllPath(modulePath);
    std::filesystem::path dllDirectory = dllPath.parent_path();

    if (dllDirectory.filename().wstring() == L"resources") {
        const std::filesystem::path appRoot = dllDirectory.parent_path();
        if (!appRoot.empty()) {
            return appRoot;
        }
    }

    return dllDirectory;
}
} // anonymous namespace

void Init(HMODULE selfModule) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_file) return;

    const std::filesystem::path appRoot = ResolveAppRoot(selfModule);
    const std::filesystem::path logsDirectory = appRoot / "logs";
    std::error_code createError;
    std::filesystem::create_directories(logsDirectory, createError);

    // Create timestamped file
    SYSTEMTIME t;
    GetLocalTime(&t);

    char timeStr[640]{};
    snprintf(timeStr, sizeof(timeStr) - 1, "%04d-%02d-%02d %02d%02d%02d", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
    const std::string logFileName = std::string("rvgl-randomizer-") + timeStr + ".log";

    const std::filesystem::path logPath = logsDirectory / logFileName;
    g_logPath = WideToUtf8(logPath.wstring());
    g_file = _wfopen(logPath.c_str(), L"w");

    const std::string msg = "[Logger] Logging to " +
        (g_logPath.empty() ? std::string("rvgl-randomizer.log") : g_logPath) + "\n";
    OutputDebugStringA(msg.c_str());
    if (g_file) {
        fputs(msg.c_str(), g_file);
        fflush(g_file);
    }
}

void Shutdown() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_file) {
        fclose(g_file);
        g_file = nullptr;
    }
    g_logPath.clear();
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
