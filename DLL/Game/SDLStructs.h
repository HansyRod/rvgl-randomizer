#pragma once

#include <cstddef>
#include <cstdint>

// SDL2 ABI prefixes used here; the game supplies SDL2.dll, so no SDK or new
// runtime dependency is required. SDL's window and mutex objects stay opaque.

// SDL_DisplayMode layout for the project's Windows x64 target. Keep the native
// alignment: the driver-data pointer starts at +0x10 and the structure is 0x18 bytes.
struct DesktopDisplayMode {
    uint32_t format;                             // +0x00, SDL pixel-format identifier
    int width;                                  // +0x04, display width in pixels
    int height;                                 // +0x08, display height in pixels
    int refreshRate;                             // +0x0c, refresh rate in Hz
    void* driverData;                            // +0x10, opaque SDL driver data
};

static_assert(offsetof(DesktopDisplayMode, driverData) == 0x10, "SDL display-mode layout mismatch.");
static_assert(sizeof(DesktopDisplayMode) == 0x18, "SDL display-mode size mismatch.");
