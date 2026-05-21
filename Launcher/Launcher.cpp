#include "Launcher.h"

#include <cctype>
#include <filesystem>
#include <string>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <sys/types.h>
#include <unistd.h>
#endif

// ============================================================================
// Internal helpers — not exposed to callers
// ============================================================================

namespace {

// Returns the directory portion of a full file path, with no trailing slash.
// "C:\Games\RVGL\rvgl.exe" -> "C:\Games\RVGL"
std::string GetDirectoryFromPath(const std::string& fullPath) {
    const size_t slash = fullPath.find_last_of("\\/");
    return (slash != std::string::npos) ? fullPath.substr(0, slash) : ".";
}


// Builds a null-terminated command line buffer accepted by CreateProcessA.
// CreateProcessA may modify the buffer, so we cannot pass a string literal.
std::string BuildCommandLine(const std::string& exePath, const std::string& extraArgs) {
    // Wrap the exe path in quotes to handle paths with spaces.
    std::string cmd = "\"" + exePath + "\"";
    if (!extraArgs.empty()) {
        cmd += " ";
        cmd += extraArgs;
    }
    return cmd;
}

#if defined(_WIN32)
// Injects a DLL into a running (or suspended) process by the standard
// VirtualAllocEx + WriteProcessMemory + CreateRemoteThread(LoadLibraryA) method.
//
// hProcess must have been opened (or created) with at least:
//   PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE
//
// Returns true if LoadLibrary succeeded inside the target process.
bool InjectDll(HANDLE hProcess, const std::string& dllPath) {
    const SIZE_T pathLen = dllPath.size() + 1;  // include null terminator

    // Allocate memory for the DLL path string inside RVGL's address space
    LPVOID remotePath = VirtualAllocEx(
        hProcess, nullptr, pathLen,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if (!remotePath)
        return false;

    // Write the path string into RVGL's memory
    if (!WriteProcessMemory(hProcess, remotePath, dllPath.c_str(), pathLen, nullptr)) {
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
        return false;
    }

    // kernel32.dll is loaded at the same address in every process on the same
    // OS session, so the LoadLibraryA address we look up here is valid to use
    // directly in RVGL's address space.
    HMODULE k32 = GetModuleHandleA("kernel32.dll");
    if (!k32) {
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
        return false;
    }

    auto loadLibraryA = reinterpret_cast<LPTHREAD_START_ROUTINE>(
        GetProcAddress(k32, "LoadLibraryA"));

    if (!loadLibraryA) {
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
        return false;
    }

    // Spin up a thread inside RVGL that calls LoadLibraryA(dllPath).
    // Our DllMain fires and installs hooks before this thread returns.
    HANDLE hRemoteThread = CreateRemoteThread(
        hProcess, nullptr, 0,
        loadLibraryA, remotePath,
        0, nullptr);

    if (!hRemoteThread) {
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
        return false;
    }

    // Block until LoadLibrary (and therefore our DllMain) has finished.
    // RVGL's main thread is still suspended at this point, so there is
    // no race between hook installation and game code executing.
    WaitForSingleObject(hRemoteThread, INFINITE);

    DWORD remoteResult = 0;
    GetExitCodeThread(hRemoteThread, &remoteResult);

    CloseHandle(hRemoteThread);
    VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);

    // LoadLibraryA returns the HMODULE on success, NULL (0) on failure.
    return remoteResult != 0;
}
#endif

#if !defined(_WIN32)
std::vector<std::string> SplitExtraArgs(const std::string& args) {
    std::vector<std::string> result;
    std::string current;
    bool inQuotes = false;

    for (char ch : args) {
        if (ch == '"') {
            inQuotes = !inQuotes;
            continue;
        }

        if (!inQuotes && std::isspace(static_cast<unsigned char>(ch))) {
            if (!current.empty()) {
                result.push_back(current);
                current.clear();
            }
            continue;
        }

        current.push_back(ch);
    }

    if (!current.empty()) {
        result.push_back(current);
    }

    return result;
}
#endif

} // anonymous namespace

// ============================================================================
// Public API
// ============================================================================

namespace Launcher {

LaunchResult Launch(const LaunchConfig& config) {
    // --- Validate inputs before touching any OS resources ---

    if (config.rvglExePath.empty())
        return { false, "rvglExePath is empty.", 0 };

    if (config.modLibraryPath.empty())
        return { false, "modLibraryPath is empty.", 0 };

#if defined(_WIN32)
    if (!config.configPath.empty()) {
        SetEnvironmentVariableA("RVGL_RANDOMIZER_CONFIG", config.configPath.c_str());
    }

    // --- Start RVGL suspended so no game code runs before we inject ---

    STARTUPINFOA        si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    std::string cmdLine = BuildCommandLine(config.rvglExePath, config.extraArgs);

    const std::string workingDir = GetDirectoryFromPath(config.rvglExePath);
    const bool created = CreateProcessA(
        config.rvglExePath.c_str(),  // executable (used to find the binary)
        cmdLine.data(),              // modifiable command line
        nullptr,                     // default process security
        nullptr,                     // default thread security
        false,                       // do not inherit handles
        CREATE_SUSPENDED,            // main thread does not run yet
        nullptr,                     // inherit parent environment
        workingDir.c_str(),          // use RVGL folder as working directory
        &si,
        &pi);

    if (!created) {
        const DWORD err = GetLastError();
        return { false, "CreateProcess failed. Windows error: " + std::to_string(err), 0 };
    }

    // --- Inject the mod while the main thread is still suspended ---

    const bool injected = InjectDll(pi.hProcess, config.modLibraryPath);

    if (!injected) {
        // Don't leave a suspended RVGL process hanging if injection fails.
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return { false, "DLL injection failed. Check that the mod library path is correct "
                        "and that the process has sufficient permissions.", 0 };
    }

    // --- Hooks are installed; let RVGL run ---

    ResumeThread(pi.hThread);

    const std::uint32_t pid = pi.dwProcessId;

    // We don't need to track the process after launch —
    // the caller can hold onto the PID if it wants to.
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return { true, "", pid };
#else
    const pid_t child = fork();
    if (child < 0) {
        return { false, "fork failed.", 0 };
    }

    if (child == 0) {
        const std::string workingDir = GetDirectoryFromPath(config.rvglExePath);
        chdir(workingDir.c_str());

        setenv("LD_PRELOAD", config.modLibraryPath.c_str(), 1);
        if (!config.configPath.empty()) {
            setenv("RVGL_RANDOMIZER_CONFIG", config.configPath.c_str(), 1);
        }

        std::vector<std::string> args;
        args.push_back(config.rvglExePath);
        for (const std::string& arg : SplitExtraArgs(config.extraArgs)) {
            args.push_back(arg);
        }

        std::vector<char*> argv;
        argv.reserve(args.size() + 1);
        for (std::string& arg : args) {
            argv.push_back(arg.data());
        }
        argv.push_back(nullptr);

        execv(config.rvglExePath.c_str(), argv.data());
        _exit(127);
    }

    return { true, "", static_cast<std::uint32_t>(child) };
#endif
}

} // namespace Launcher