#pragma once

#include "SDLStructs.h"
#include <cstdint>

namespace Randomizer {

using FnSDLGetKeyboardState = const uint8_t*(*)(int* keyCount);
using FnSDLGetModState = int(*)();
using FnSDLGetClipboardText = char*(*)();
using FnSDLFree = void(*)(void* memory);
using FnSDLGetWindowFlags = uint32_t(*)(void* window);
using FnSDLGetWindowDisplayIndex = int(*)(void* window);
using FnSDLGetDesktopDisplayMode = int(*)(int displayIndex, DesktopDisplayMode* mode);
using FnSDLGetError = const char*(*)();
using FnSDLLockMutex = int(*)(void* mutex);
using FnSDLUnlockMutex = int(*)(void* mutex);
using FnSDLMinimizeWindow = void(*)(void* window);

extern FnSDLGetKeyboardState RVGL_SDLGetKeyboardState;
extern FnSDLGetModState RVGL_SDLGetModState;
extern FnSDLGetClipboardText RVGL_SDLGetClipboardText;
extern FnSDLFree RVGL_SDLFree;
extern FnSDLGetWindowFlags RVGL_SDLGetWindowFlags;
extern FnSDLGetWindowDisplayIndex RVGL_SDLGetWindowDisplayIndex;
extern FnSDLGetDesktopDisplayMode RVGL_SDLGetDesktopDisplayMode;
extern FnSDLGetError RVGL_SDLGetError;
extern FnSDLLockMutex RVGL_SDLLockMutex;
extern FnSDLUnlockMutex RVGL_SDLUnlockMutex;
extern FnSDLMinimizeWindow RVGL_SDLMinimizeWindow;

} // namespace Randomizer
