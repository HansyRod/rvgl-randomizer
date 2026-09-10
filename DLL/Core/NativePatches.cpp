#include "NativePatches.h"
#include "PatchAddresses.h"
#include "Logger.h"
#include <windows.h>
#include <cstring>
#include <string>
#include <vector>

namespace NativePatches {

namespace {

struct PatchEntry {
    uint32_t rva;
    std::vector<uint8_t> expectedBytes;
    std::vector<uint8_t> replacementBytes;
    std::string name;
    bool applied = false;
};

std::vector<PatchEntry> g_patches;
bool g_patchesRegistered = false;

void RegisterPatches() {
    // LoadCustomCarPool passes strlen(candidateName) to the native comparison
    // helper. That makes a short custom folder such as "hs" compare equal to
    // the prefix of a vanilla folder such as "hsf1". At this point R13 still
    // contains candidateName, so using it as the maximum length makes the
    // helper compare the complete strings. The replacement is the same size
    // as the original instruction and needs no trampoline or code cave.
    constexpr uint8_t expectedBytes[] = { 0x49, 0x89, 0xC0 }; // mov r8, rax
    constexpr uint8_t replacementBytes[] = { 0x4D, 0x89, 0xE8 }; // mov r8, r13

    Add(
        RVA_CUSTOM_CAR_NAME_COMPARE_LENGTH,
        expectedBytes,
        replacementBytes,
        sizeof(expectedBytes),
        "CustomCarNameComparison"
    );
}

bool WriteBytes(uint8_t* address, const std::vector<uint8_t>& bytes) {
    DWORD oldProtection = 0;
    if (!VirtualProtect(
            address,
            bytes.size(),
            PAGE_EXECUTE_READWRITE,
            &oldProtection
        )) {
        return false;
    }

    std::memcpy(address, bytes.data(), bytes.size());
    const BOOL flushed = FlushInstructionCache(
        GetCurrentProcess(),
        address,
        bytes.size()
    );

    DWORD ignoredProtection = 0;
    const BOOL restored = VirtualProtect(
        address,
        bytes.size(),
        oldProtection,
        &ignoredProtection
    );

    if (flushed == FALSE) {
        Logger::TimestampLogf("[NativePatches] Warning: instruction-cache flush failed");
    }
    if (restored == FALSE) {
        Logger::TimestampLogf("[NativePatches] Warning: page protection restore failed");
    }

    // The bytes were written successfully. Keep the patch marked as applied so
    // RemoveAll can still attempt to restore it if cleanup reported a warning.
    return true;
}

bool ApplyPatch(PatchEntry& patch) {
    HMODULE module = GetModuleHandleA("rvgl.exe");
    if (module == nullptr) {
        Logger::TimestampLogf(
            "[NativePatches] Refusing %s: rvgl.exe is not loaded",
            patch.name.c_str()
        );
        return false;
    }

    auto* address = reinterpret_cast<uint8_t*>(
        reinterpret_cast<uintptr_t>(module) + patch.rva
    );

    if (std::memcmp(address, patch.replacementBytes.data(), patch.replacementBytes.size()) == 0) {
        patch.applied = true;
        Logger::TimestampLogf(
            "[NativePatches] Already applied: %s",
            patch.name.c_str()
        );
        return true;
    }

    if (std::memcmp(address, patch.expectedBytes.data(), patch.expectedBytes.size()) != 0) {
        Logger::TimestampLogf(
            "[NativePatches] Refusing %s: unexpected bytes at RVA 0x%08X",
            patch.name.c_str(),
            patch.rva
        );
        return false;
    }

    if (!WriteBytes(address, patch.replacementBytes)) {
        Logger::TimestampLogf(
            "[NativePatches] Failed to apply: %s",
            patch.name.c_str()
        );
        return false;
    }

    patch.applied = true;
    Logger::TimestampLogf(
        "[NativePatches] Applied: %s at RVA 0x%08X",
        patch.name.c_str(),
        patch.rva
    );
    return true;
}

} // namespace

bool Add(
    uint32_t rva,
    const uint8_t* expectedBytes,
    const uint8_t* replacementBytes,
    std::size_t size,
    const char* name
) {
    if (rva == 0 || expectedBytes == nullptr || replacementBytes == nullptr ||
        size == 0 || name == nullptr || *name == '\0') {
        Logger::TimestampLogf("[NativePatches] Refusing invalid patch definition");
        return false;
    }

    g_patches.push_back({
        rva,
        std::vector<uint8_t>(expectedBytes, expectedBytes + size),
        std::vector<uint8_t>(replacementBytes, replacementBytes + size),
        name,
        false
    });
    return true;
}

bool InstallAll() {
    if (!g_patchesRegistered) {
        RegisterPatches();
        g_patchesRegistered = true;
    }

    bool allApplied = true;
    for (PatchEntry& patch : g_patches) {
        if (!ApplyPatch(patch)) {
            allApplied = false;
        }
    }

    Logger::TimestampLogf(
        "[NativePatches] InstallAll complete — %zu patch(es), success=%s",
        g_patches.size(),
        allApplied ? "true" : "false"
    );
    return allApplied;
}

void RemoveAll() {
    HMODULE module = GetModuleHandleA("rvgl.exe");
    if (module == nullptr) {
        return;
    }

    for (PatchEntry& patch : g_patches) {
        if (!patch.applied) {
            continue;
        }

        auto* address = reinterpret_cast<uint8_t*>(
            reinterpret_cast<uintptr_t>(module) + patch.rva
        );

        if (std::memcmp(address, patch.replacementBytes.data(), patch.replacementBytes.size()) != 0) {
            Logger::TimestampLogf(
                "[NativePatches] Refusing to restore %s: bytes changed after patch",
                patch.name.c_str()
            );
            continue;
        }

        if (WriteBytes(address, patch.expectedBytes)) {
            Logger::TimestampLogf(
                "[NativePatches] Restored: %s",
                patch.name.c_str()
            );
            patch.applied = false;
        }
        else {
            Logger::TimestampLogf(
                "[NativePatches] Failed to restore: %s",
                patch.name.c_str()
            );
        }
    }
}

} // namespace NativePatches
