#pragma once

#include <cstddef>
#include <cstdint>

// ============================================================================
// NativePatches
//
// Small, version-guarded byte patches for native RVGL instructions. Patches
// are registered once and applied during DLL startup by DllMain, separately
// from the MinHook registrations managed by HookManager.
// ============================================================================

namespace NativePatches {

    // Register one exact byte replacement at an RVGL-relative address.
    // `expectedBytes` and `replacementBytes` must each contain `size` bytes.
    // The expected bytes are checked before writing so an incompatible RVGL
    // executable is left untouched.
    bool Add(
        uint32_t rva,
        const uint8_t* expectedBytes,
        const uint8_t* replacementBytes,
        std::size_t size,
        const char* name
    );

    // Registers and applies all native patches. Individual patch failures are
    // logged and do not prevent the remaining patches from being attempted.
    bool InstallAll();

    // Restores every patch successfully applied by InstallAll().
    void RemoveAll();

} // namespace NativePatches
