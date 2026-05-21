#include "Logger.h"
#include <filesystem>
#include "Platform.h"

#include <chrono>
#include <ctime>
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

std::filesystem::path ResolveAppRoot(Platform::ModuleHandle selfModule) {
    const std::string modulePathString = Platform::GetModulePath(selfModule);
    if (modulePathString.empty()) {
        return std::filesystem::current_path();
    }

    std::filesystem::path modulePath(modulePathString);
    std::filesystem::path moduleDirectory = modulePath.parent_path();

    if (moduleDirectory.filename().string() == "resources") {
        const std::filesystem::path appRoot = moduleDirectory.parent_path();
        if (!appRoot.empty()) {
            return appRoot;
        }
    }

    return moduleDirectory;
}

std::string CurrentTimestampForFile() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};

#if defined(_WIN32)
    localtime_s(&localTime, &nowTime);
#else
    localtime_r(&nowTime, &localTime);
#endif

    char buffer[64]{};
    std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d%02d%02d",
        localTime.tm_year + 1900,
        localTime.tm_mon + 1,
        localTime.tm_mday,
        localTime.tm_hour,
        localTime.tm_min,
        localTime.tm_sec);
    return buffer;
}

std::string CurrentTimestampForLog() {
    const auto now = std::chrono::system_clock::now();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};

#if defined(_WIN32)
    localtime_s(&localTime, &nowTime);
#else
    localtime_r(&nowTime, &localTime);
#endif

    char buffer[64]{};
    std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
        localTime.tm_year + 1900,
        localTime.tm_mon + 1,
        localTime.tm_mday,
        localTime.tm_hour,
        localTime.tm_min,
        localTime.tm_sec,
        static_cast<int>(millis.count()));
    return buffer;
}
} // anonymous namespace

void Init(Platform::ModuleHandle selfModule) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_file) return;

    const std::filesystem::path appRoot = ResolveAppRoot(selfModule);
    const std::filesystem::path logsDirectory = appRoot / "logs";
    std::error_code createError;
    std::filesystem::create_directories(logsDirectory, createError);

    const std::string logFileName = std::string("rvgl-randomizer-") + CurrentTimestampForFile() + ".log";
    const std::filesystem::path logPath = logsDirectory / logFileName;
    g_logPath = logPath.string();
    g_file = std::fopen(g_logPath.c_str(), "w");

    const std::string msg = "[Logger] Logging to " +
        (g_logPath.empty() ? std::string("rvgl-randomizer.log") : g_logPath) + "\n";
    Platform::DebugLog(msg.c_str());
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

    Platform::DebugLog(message);
    Platform::DebugLog("\n");

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

    const std::string timestamp = CurrentTimestampForLog();
    const std::string line = "[" + timestamp + "] " + message;
    Log(line);
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
