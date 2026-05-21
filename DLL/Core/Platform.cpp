#include "Platform.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

#if defined(_WIN32)
#include <shellapi.h>
#else
#include <cctype>
#include <cstdlib>
#include <dlfcn.h>
#include <strings.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace Platform {

namespace {

#if defined(_WIN32)
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

std::string NarrowSystemPath(const wchar_t* path) {
    return path != nullptr ? WideToUtf8(path) : std::string{};
}
#else
int ProtectionFromPerms(const std::string& perms) {
    int protection = 0;
    if (perms.size() > 0 && perms[0] == 'r') protection |= PROT_READ;
    if (perms.size() > 1 && perms[1] == 'w') protection |= PROT_WRITE;
    if (perms.size() > 2 && perms[2] == 'x') protection |= PROT_EXEC;
    return protection;
}

bool GetMemoryMapping(uintptr_t address, uintptr_t& start, uintptr_t& end, int& protection) {
    std::ifstream maps("/proc/self/maps");
    std::string line;

    while (std::getline(maps, line)) {
        std::istringstream stream(line);
        std::string range;
        std::string perms;

        if (!(stream >> range >> perms)) {
            continue;
        }

        const std::size_t dash = range.find('-');
        if (dash == std::string::npos) {
            continue;
        }

        const uintptr_t rangeStart = static_cast<uintptr_t>(std::stoull(range.substr(0, dash), nullptr, 16));
        const uintptr_t rangeEnd = static_cast<uintptr_t>(std::stoull(range.substr(dash + 1), nullptr, 16));

        if (address >= rangeStart && address < rangeEnd) {
            start = rangeStart;
            end = rangeEnd;
            protection = ProtectionFromPerms(perms);
            return true;
        }
    }

    return false;
}
#endif

} // anonymous namespace

std::string GetModulePath(ModuleHandle module) {
#if defined(_WIN32)
    wchar_t modulePath[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(module, modulePath, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return {};
    }
    return NarrowSystemPath(modulePath);
#else
    Dl_info info{};
    if (module != nullptr && dladdr(module, &info) != 0 && info.dli_fname != nullptr) {
        return info.dli_fname;
    }

    char path[4096]{};
    const ssize_t length = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (length <= 0) {
        return {};
    }

    path[length] = '\0';
    return path;
#endif
}

std::string GetCurrentDirectoryString() {
    return std::filesystem::current_path().string();
}

std::vector<std::string> GetProcessArguments() {
#if defined(_WIN32)
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<std::string> result;

    if (argv != nullptr) {
        result.reserve(static_cast<std::size_t>(argc));
        for (int i = 0; i < argc; ++i) {
            result.push_back(WideToUtf8(argv[i]));
        }
        LocalFree(argv);
    }

    return result;
#else
    std::ifstream cmdline("/proc/self/cmdline", std::ios::binary);
    std::vector<std::string> result;
    std::string current;
    char ch = '\0';

    while (cmdline.get(ch)) {
        if (ch == '\0') {
            result.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }

    if (!current.empty()) {
        result.push_back(current);
    }

    return result;
#endif
}

bool FileExists(const std::string& path) {
    std::error_code error;
    return std::filesystem::exists(path, error);
}

bool UnsetEnv(const char* name) {
#if defined(_WIN32)
    return SetEnvironmentVariableA(name, nullptr) != 0;
#else
    return unsetenv(name) == 0;
#endif
}

void DebugLog(const char* message) {
    if (message == nullptr) {
        return;
    }

#if defined(_WIN32)
    OutputDebugStringA(message);
#else
    fputs(message, stderr);
#endif
}

int CaseInsensitiveCompare(const char* left, const char* right) {
    if (left == nullptr || right == nullptr) {
        return left == right ? 0 : (left == nullptr ? -1 : 1);
    }

#if defined(_WIN32)
    return _stricmp(left, right);
#else
    return strcasecmp(left, right);
#endif
}

int CaseInsensitiveNCompare(const char* left, const char* right, std::size_t count) {
    if (left == nullptr || right == nullptr) {
        return left == right ? 0 : (left == nullptr ? -1 : 1);
    }

#if defined(_WIN32)
    return _strnicmp(left, right, count);
#else
    return strncasecmp(left, right, count);
#endif
}

void CopyTruncated(char* dest, std::size_t destSize, const char* src) {
    if (dest == nullptr || destSize == 0) {
        return;
    }

    if (src == nullptr) {
        dest[0] = '\0';
        return;
    }

    std::strncpy(dest, src, destSize - 1);
    dest[destSize - 1] = '\0';
}

bool WriteProtectedMemory(void* dest, const void* src, std::size_t size) {
    if (dest == nullptr || src == nullptr || size == 0) {
        return false;
    }

#if defined(_WIN32)
    DWORD oldProtection = 0;
    if (!VirtualProtect(dest, size, PAGE_READWRITE, &oldProtection)) {
        return false;
    }

    std::memcpy(dest, src, size);

    DWORD ignored = 0;
    VirtualProtect(dest, size, oldProtection, &ignored);
    return true;
#else
    uintptr_t mappingStart = 0;
    uintptr_t mappingEnd = 0;
    int oldProtection = 0;
    const uintptr_t writeStart = reinterpret_cast<uintptr_t>(dest);

    if (!GetMemoryMapping(writeStart, mappingStart, mappingEnd, oldProtection)) {
        return false;
    }

    const long pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize <= 0) {
        return false;
    }

    const uintptr_t pageMask = static_cast<uintptr_t>(pageSize - 1);
    const uintptr_t protectStart = writeStart & ~pageMask;
    const uintptr_t protectEnd = (writeStart + size + pageMask) & ~pageMask;
    const std::size_t protectSize = protectEnd - protectStart;
    const int writeProtection = oldProtection | PROT_WRITE;

    if (mprotect(reinterpret_cast<void*>(protectStart), protectSize, writeProtection) != 0) {
        return false;
    }

    std::memcpy(dest, src, size);

    mprotect(reinterpret_cast<void*>(protectStart), protectSize, oldProtection);
    return true;
#endif
}

} // namespace Platform
