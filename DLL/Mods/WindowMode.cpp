#include "WindowMode.h"
#include "Addresses.h"
#include "LocaleUtils.h"
#include "Logger.h"
#include "ProfileHooks.h"
#include "RVGLFunctions.h"
#include "RVGLMemory.h"
#include "SDLFunctions.h"
#include <cstdio>
#include <cstring>
#include <intrin.h>

namespace Randomizer {

FnApplyWindowMode Orig_ApplyWindowMode = nullptr;
FnBuildVideoSettingsMenu Orig_BuildVideoSettingsMenu = nullptr;
FnDrawWindowModeMenuRow Orig_DrawWindowModeMenuRow = nullptr;
FnMinimizeGameWindow Orig_MinimizeGameWindow = nullptr;

namespace {

enum class WindowMode { FullScreen, Windowed, Borderless };

constexpr uint32_t kSDLFullscreen = 1;
// SDL_WINDOW_FULLSCREEN_DESKTOP combines the fullscreen bit with 0x1000,
// which selects the existing desktop mode instead of changing display modes.
constexpr uint32_t kSDLFullscreenDesktop = kSDLFullscreen | 0x1000;
constexpr int kResolutionLabelId = 0x49;
constexpr char kWindowModeLabel[] = "Window Mode";
constexpr const char* kModeNames[] = { "Full Screen", "Windowed", "Borderless" };

bool g_initialized = false;
bool g_launchWindowed = false;
bool g_profileLoaded = false;
WindowMode g_mode = WindowMode::FullScreen;
WindowModeProfileSettings g_profileSettings;
MenuItemDescriptor g_modeMenuItem = {};
MenuItemDescriptor g_resolutionMenuItem = {};

void LoadWindowMode() {
    g_initialized = true;
    g_mode = GetFullscreenFlag() != 0
        ? WindowMode::FullScreen : WindowMode::Windowed;

    // ProfileHooks resolves the per-profile mod INI through RVGL's VFS, so this
    // follows -prefpath. Graphics may initialize before that profile is loaded.
    const int savedMode = g_profileSettings.mode;
    if (!g_launchWindowed && savedMode >= 0) {
        g_mode = static_cast<WindowMode>(savedMode);
        GetFullscreenFlag() = g_mode != WindowMode::Windowed;
    }
}

void SaveWindowMode() {
    g_profileSettings.mode = static_cast<int>(g_mode);
    if (!SaveActiveRandomizerProfileSettings()) {
        Logger::TimestampLog("[WindowMode] Could not save the active profile's window mode.");
    }
}

bool ApplyNativeMode(WindowMode mode) {
    const bool requestedFullscreen = GetFullscreenFlag() != 0;
    uint8_t& noResolutionChange = GetNoresFlag();
    const uint8_t savedNores = noResolutionChange;
    // Keep the game's native placement, display selection, context and windowed
    // geometry handling. The override lasts only for this call, never for F11
    // or the user's command-line preference after leaving Borderless.
    if (mode == WindowMode::Borderless) {
        noResolutionChange = 1;
    }
    Orig_ApplyWindowMode();
    noResolutionChange = savedNores;

    const uint32_t flags = RVGL_SDLGetWindowFlags(GetWindow());
    if (!requestedFullscreen) {
        return (flags & kSDLFullscreen) == 0;
    }
    if (GetFullscreenFlag() == 0) {
        // The native helper falls back to Windowed when it has no display
        // modes. Do not report that fallback as a successful Borderless mode.
        return false;
    }
    return mode == WindowMode::Borderless
        ? (flags & kSDLFullscreenDesktop) == kSDLFullscreenDesktop
        : (flags & kSDLFullscreen) != 0;
}

bool ApplySelectedMode(WindowMode requestedMode) {
    const WindowMode previousMode = g_mode;
    if (!ApplyNativeMode(requestedMode)) {
        Logger::TimestampLogf("[WindowMode] Could not apply %s: %s. Restoring %s.",
            kModeNames[static_cast<int>(requestedMode)], RVGL_SDLGetError(),
            kModeNames[static_cast<int>(previousMode)]);
        GetFullscreenFlag() = previousMode != WindowMode::Windowed;
        if (!ApplyNativeMode(previousMode)) {
            Logger::TimestampLog("[WindowMode] Restore failed; falling back to Windowed.");
            GetFullscreenFlag() = 0;
            Orig_ApplyWindowMode();
            g_mode = WindowMode::Windowed;
        }
        return false;
    }
    g_mode = requestedMode;
    Logger::TimestampLogf("[WindowMode] Applied %s.", kModeNames[static_cast<int>(g_mode)]);
    return true;
}

void RefreshWindowLayout() {
    // Match F11's synchronization with the loading/render thread.
    // Called after our menu/profile transitions. Native F11 performs its own
    // refresh; focus changes alone do not call this helper.
    void* mutex = GetRenderMutex();
    if (mutex != nullptr && RVGL_SDLLockMutex(mutex) != 0) {
        Logger::TimestampLogf("[WindowMode] Could not lock render mutex: %s", RVGL_SDLGetError());
        return;
    }
    if (mutex != nullptr) {
        GetRenderUpdatingFlag() = 1;
    }
    RVGL_RefreshWindowLayout();
    if (mutex != nullptr) {
        GetRenderUpdatingFlag() = 0;
        RVGL_SDLUnlockMutex(mutex);
    }
}

bool CycleWindowMode(int direction) {
    // Left/Right select the previous/next mode, wrapping at either end of the
    // three choices. 'requested' is the candidate; g_mode changes only after
    // the native transition succeeds. Then refresh the viewport/menu geometry
    // and save the successful selection in the active profile's mod INI.
    int nextMode = static_cast<int>(g_mode) + direction;
    if (nextMode < 0) {
        nextMode = 2;
    } else if (nextMode > 2) {
        nextMode = 0;
    }
    const WindowMode requested = static_cast<WindowMode>(nextMode);
    GetFullscreenFlag() = requested != WindowMode::Windowed;
    const bool changed = ApplySelectedMode(requested);
    RefreshWindowLayout();
    if (changed) {
        SaveWindowMode();
    }
    return changed;
}

bool DecrementWindowMode(int) { return CycleWindowMode(-1); }
bool IncrementWindowMode(int) { return CycleWindowMode(1); }

void DrawValue(int slotIndex, int itemIndex, const char* text, uint32_t color) {
    const MenuPanelSlot* slot = GetMenuPanelSlot(slotIndex);
    if (slot != nullptr) {
        RVGL_DrawUIText(slot->textX + slot->labelWidth + 32.0f,
            slot->textY + static_cast<float>(itemIndex * 16), 8.0f, 12.0f,
            color, const_cast<char*>(text), slot->valueWidth, 0);
    }
}

void DrawWindowModeValue(int slotIndex, int itemIndex) {
    DrawValue(slotIndex, itemIndex, kModeNames[static_cast<int>(g_mode)], 0xffebebeb);
}

void DrawResolutionValue(int slotIndex, int itemIndex) {
    if (g_mode != WindowMode::Borderless) {
        GetNativeResolutionItem()->drawValue(slotIndex, itemIndex);
        return;
    }
    DesktopDisplayMode desktop = {};
    const int monitor = RVGL_SDLGetWindowDisplayIndex(GetWindow());
    char text[64] = "Desktop";
    if (monitor >= 0 && RVGL_SDLGetDesktopDisplayMode(monitor, &desktop) == 0) {
        std::snprintf(text, sizeof(text), "%dx%d,%dHz", desktop.width, desktop.height, desktop.refreshRate);
    }
    DrawValue(slotIndex, itemIndex, text, 0xff464646);
}

bool DecrementResolution(int slotIndex) {
    if (g_mode == WindowMode::Borderless) {
        return false;
    } else {
        return GetNativeResolutionItem()->decrementValue(slotIndex);
    }
}

bool IncrementResolution(int slotIndex) {
    if (g_mode == WindowMode::Borderless) {
        return false;
    } else {
        return GetNativeResolutionItem()->incrementValue(slotIndex);
    }
}

} // anonymous namespace

WindowModeProfileSettings& GetWindowModeProfileSettings() {
    return g_profileSettings;
}

void ApplyWindowModeProfileSettings() {
    // An explicit -window takes precedence over the startup profile's saved
    // mode. Switching profiles later uses that profile's preference normally.
    const bool startupProfile = !g_profileLoaded;
    g_profileLoaded = true;
    if ((startupProfile && g_launchWindowed) || g_profileSettings.mode < 0 || GetWindow() == nullptr) {
        return;
    }
    const WindowMode requested = static_cast<WindowMode>(g_profileSettings.mode);
    if (g_initialized && requested == g_mode) {
        return;
    }
    if (!g_initialized) {
        LoadWindowMode();
    }
    GetFullscreenFlag() = requested != WindowMode::Windowed;
    ApplySelectedMode(requested);
    RefreshWindowLayout();
}

void ConfigureWindowModeLaunch(int argc, char** argv) {
    for (int index = 1; index < argc; ++index) {
        if (argv[index] != nullptr && std::strcmp(argv[index], "-window") == 0) {
            g_launchWindowed = true;
        }
    }
}

void Hook_ApplyWindowMode() {
    if (GetWindow() == nullptr) {
        Orig_ApplyWindowMode();
        return;
    }
    if (!g_initialized) {
        LoadWindowMode();
    }
    // _ReturnAddress is the instruction that will execute when this hook
    // returns. Comparing it with the instruction after F11's native call
    // identifies that caller; it does not read the keyboard state.
    const bool f11 = reinterpret_cast<uintptr_t>(_ReturnAddress()) == AbsFromRva(RVA_F11_APPLY_MODE_RETURN);
    // Dialogs also toggle the fullscreen byte. Only the native F11 call changes
    // the selected mode; a dialog must restore Borderless after it closes.
    if (f11) {
        const WindowMode requested = GetFullscreenFlag() != 0
            ? WindowMode::FullScreen : WindowMode::Windowed;
        if (ApplySelectedMode(requested)) {
            SaveWindowMode();
        }
    } else {
        ApplySelectedMode(g_mode);
    }
}

void Hook_MinimizeGameWindow(void* window) {
    // Keep native focus tracking and -nopause handling intact; only suppress
    // the automatic minimize operation, never an explicit user minimize.
    if (g_mode == WindowMode::Borderless && window == GetWindow() &&
        reinterpret_cast<uintptr_t>(_ReturnAddress()) == AbsFromRva(RVA_FOCUS_LOSS_MINIMIZE_RETURN)) {
        return;
    }
    Orig_MinimizeGameWindow(window);
}

void Hook_BuildVideoSettingsMenu(int slotIndex) {
    Orig_BuildVideoSettingsMenu(slotIndex);
    MenuPanelSlot* slot = GetMenuPanelSlot(slotIndex);
    // MenuPanelSlot::items has 30 entries; native registration appends one.
    if (slot == nullptr || slot->itemCount >= 30) {
        return;
    }
    for (int index = 0; index < slot->itemCount; ++index) {
        if (slot->items[index] != GetNativeResolutionItem()) {
            continue;
        }
        g_resolutionMenuItem = *GetNativeResolutionItem();
        g_resolutionMenuItem.drawValue = DrawResolutionValue;
        g_resolutionMenuItem.decrementValue = DecrementResolution;
        g_resolutionMenuItem.incrementValue = IncrementResolution;
        slot->items[index] = &g_resolutionMenuItem;
        g_modeMenuItem = {};
        g_modeMenuItem.labelId = kResolutionLabelId;
        g_modeMenuItem.flags.selectable = 1;
        g_modeMenuItem.flags.enabled = 1;
        g_modeMenuItem.valueWidth = 88.0f;
        g_modeMenuItem.drawValue = DrawWindowModeValue;
        g_modeMenuItem.decrementValue = DecrementWindowMode;
        g_modeMenuItem.incrementValue = IncrementWindowMode;
        // Borrow Resolution's locale entry only during measurement/drawing of
        // our own row. No override survives into another row or exit animation.
        LocaleStringOverride modeLabel{kResolutionLabelId, kWindowModeLabel};
        OverrideLocaleString(modeLabel);
        RVGL_RegisterMenuItemInActiveMenu(slotIndex, reinterpret_cast<int*>(&g_modeMenuItem));
        RestoreLocaleString(modeLabel);
        for (int row = slot->itemCount - 1; row > index + 1; --row) {
            slot->items[row] = slot->items[row - 1];
        }
        slot->items[index + 1] = &g_modeMenuItem;
        return;
    }
}

void Hook_DrawWindowModeMenuRow(int slotIndex, uint32_t itemIndex, uint32_t visibleIndex) {
    const MenuPanelSlot* slot = GetMenuPanelSlot(slotIndex);
    const bool customRow = slot != nullptr && itemIndex < static_cast<uint32_t>(slot->itemCount) &&
        itemIndex < 30 && slot->items[itemIndex] == &g_modeMenuItem;
    LocaleStringOverride modeLabel{kResolutionLabelId, kWindowModeLabel};
    if (customRow) {
        OverrideLocaleString(modeLabel);
    }
    Orig_DrawWindowModeMenuRow(slotIndex, itemIndex, visibleIndex);
    if (customRow) {
        RestoreLocaleString(modeLabel);
    }
}

} // namespace Randomizer
