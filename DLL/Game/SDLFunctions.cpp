#include "SDLFunctions.h"
#include "Addresses.h"

namespace Randomizer {

namespace {

template<typename T>
T ResolveSDLFunction(const char* name) {
    // SDL2 is already a dependency of RVGL. Bind its exports without loading a
    // second library or adding an SDL SDK dependency to the injected DLL.
    // Only needed for functions that RVGL does not import and has no thunk for.
    HMODULE sdl = GetModuleHandleA("SDL2.dll");
    return sdl != nullptr ? reinterpret_cast<T>(GetProcAddress(sdl, name)) : nullptr;
}

} // anonymous namespace

FnSDLGetKeyboardState       RVGL_SDLGetKeyboardState        = reinterpret_cast<FnSDLGetKeyboardState>(AbsFromRva(RVA_SDL_GET_KEYBOARD_STATE));
FnSDLGetModState            RVGL_SDLGetModState             = reinterpret_cast<FnSDLGetModState>(AbsFromRva(RVA_SDL_GET_MOD_STATE));
FnSDLGetClipboardText       RVGL_SDLGetClipboardText        = reinterpret_cast<FnSDLGetClipboardText>(AbsFromRva(RVA_SDL_GET_CLIPBOARD_TEXT));
FnSDLFree                   RVGL_SDLFree                    = reinterpret_cast<FnSDLFree>(AbsFromRva(RVA_SDL_FREE));
FnSDLGetWindowFlags         RVGL_SDLGetWindowFlags          = ResolveSDLFunction<FnSDLGetWindowFlags>("SDL_GetWindowFlags");
FnSDLGetWindowDisplayIndex  RVGL_SDLGetWindowDisplayIndex   = ResolveSDLFunction<FnSDLGetWindowDisplayIndex>("SDL_GetWindowDisplayIndex");
FnSDLGetDesktopDisplayMode  RVGL_SDLGetDesktopDisplayMode   = reinterpret_cast<FnSDLGetDesktopDisplayMode>(AbsFromRva(RVA_SDL_GET_DESKTOP_DISPLAY_MODE));
FnSDLGetError               RVGL_SDLGetError                = reinterpret_cast<FnSDLGetError>(AbsFromRva(RVA_SDL_GET_ERROR));
FnSDLLockMutex              RVGL_SDLLockMutex               = reinterpret_cast<FnSDLLockMutex>(AbsFromRva(RVA_SDL_LOCK_MUTEX));
FnSDLUnlockMutex            RVGL_SDLUnlockMutex             = reinterpret_cast<FnSDLUnlockMutex>(AbsFromRva(RVA_SDL_UNLOCK_MUTEX));
FnSDLMinimizeWindow         RVGL_SDLMinimizeWindow          = reinterpret_cast<FnSDLMinimizeWindow>(AbsFromRva(RVA_SDL_MINIMIZE_WINDOW));

} // namespace Randomizer
