#include "CallLogger.h"
#include "Logger.h"
#include <array>
#include <cstdint>
#include <unordered_map>
#include <utility>   // std::index_sequence, std::make_index_sequence
#if defined(_WIN32)
#include "RVGLAddresses.h"
#include "MinHook.h"
#include <windows.h>
#endif

#if !defined(_DEBUG) || !defined(_WIN32)

namespace CallLogger {

int RegisterAll(const std::unordered_map<uint32_t, const char*>&) {
    return 0;
}

void UnregisterAll() {
}

} // namespace CallLogger

#else

// ============================================================================
// How the template pool trick works
// ============================================================================
//
// MinHook requires a unique *function address* per hook. You can't install the
// same detour function on ten different targets — MinHook uses the address as
// a key, and your code inside the detour needs to know which slot it belongs
// to in order to look up the right name and original pointer.
//
// Solution: pre-generate N distinct functions at compile time using a template
// parameter as a slot index.
//
//   template<size_t Slot>
//   void* __fastcall Detour(void* a, void* b, void* c, void* d) { ... }
//
// Detour<0>, Detour<1>, ..., Detour<63> are all distinct functions with
// distinct addresses. Each reads its name and original pointer from the
// corresponding entry in g_slots[Slot].
//
// std::index_sequence is used to expand all N specializations into an array
// of function pointers at program startup (g_detourTable), so registration
// just picks the next unused slot.
// ============================================================================

namespace CallLogger {

// ----------------------------------------------------------------------------
// Slot storage
// ----------------------------------------------------------------------------

struct HookSlot {
    const char* name     = nullptr;  // display name for the log
    void*       original = nullptr;  // MinHook writes the trampoline here
    void*       target   = nullptr;  // absolute address of the hooked function
    bool        active   = false;
};

static HookSlot g_slots[MAX_AUTO_LOG_HOOKS];
static int      g_nextSlot = 0;

// ----------------------------------------------------------------------------
// Generic detour — one instantiation per slot index
//
// Signature: void* __fastcall (void*, void*, void*, void*)
//
// This covers any RVGL function whose first four params are integers/pointers
// (RCX, RDX, R8, R9). The four args are passed straight through to the
// original so they arrive unmodified.
//
// The return value (RAX) from the original is forwarded to the caller.
// ----------------------------------------------------------------------------

using GenericFn = void* (__fastcall*)(void*, void*, void*, void*, void*, void*, void*, void*);

template<size_t Slot>
void* __fastcall GenericDetour(void* a, void* b, void* c, void* d, void* e, void* f, void* g, void* h) {
    Logger::TimestampLogf("%s(%p, %p, %p, %p, %p, %p, %p, %p)", g_slots[Slot].name, a, b, c, d, e, f, g, h);
    void* result = CallOriginalSafe(
        reinterpret_cast<GenericFn>(g_slots[Slot].original),
        a, b, c, d, e, f, g, h, g_slots[Slot].name);
    return result;
}

// ----------------------------------------------------------------------------
// Calls fn with up to 8 generic pointer/integer arguments via SEH, catching any
// access violations before they crash the process. On violation, logs the function
// name, all arguments, and the Windows exception code, then returns nullptr.
//
// The 8-parameter ceiling covers all known RVGL functions. Extra parameters beyond
// what the real function uses are harmless — the callee simply ignores registers and
// stack slots it doesn't declare. Declaring fewer than the real function needs is
// what causes crashes (stack slots for args 5+ go uninitialized), so err on the
// side of more parameters rather than fewer.
//
// NOTE: __try/__except cannot coexist with C++ objects that have destructors in the
// same function scope. Keep this function free of std::string, Logger calls, and
// any other RAII types — that is why logging is limited to the __except block only.
// ----------------------------------------------------------------------------
static void* CallOriginalSafe(GenericFn fn, void* a, void* b, void* c, void* d, void* e, void* f, void* g, void* h,
                               const char* name) {
    __try {
        return fn(a, b, c, d, e, f, g, h);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        Logger::Logf("[CallLogger] ACCESS VIOLATION: %s  "
                     "a=%p b=%p c=%p d=%p e=%p f=%p g=%p h=%p code=0x%08X",
                     name, a, b, c, d, e, f, g, h, GetExceptionCode());
        return nullptr;
    }
}

// ----------------------------------------------------------------------------
// Build the detour table at compile time
//
// MakeDetourTable expands the pack {0, 1, 2, ..., N-1} and initialises a
// std::array with the address of GenericDetour<I> for each I.
// The table is populated once at startup and never changes.
// ----------------------------------------------------------------------------

template<size_t... Is>
static constexpr std::array<GenericFn, MAX_AUTO_LOG_HOOKS>
MakeDetourTable(std::index_sequence<Is...>) {
    return { GenericDetour<Is>... };
}

static const auto g_detourTable =
    MakeDetourTable(std::make_index_sequence<MAX_AUTO_LOG_HOOKS>{});

// ----------------------------------------------------------------------------
// Public API
// ----------------------------------------------------------------------------

int RegisterAll(const std::unordered_map<uint32_t, const char*>& functions) {
    int installed = 0;

    for (const auto& [rva, name] : functions) {
        if (g_nextSlot >= MAX_AUTO_LOG_HOOKS) {
            Logger::TimestampLogf("[CallLogger] WARNING: pool full (%d slots used). "
                         "Increase MAX_AUTO_LOG_HOOKS. Skipping: %s",
                         MAX_AUTO_LOG_HOOKS, name);
            continue;
        }

        const uintptr_t target = AbsFromRva(rva);
        if (!target) {
            Logger::TimestampLogf("[CallLogger] SKIP (zero address): %s", name);
            continue;
        }

        const int slot = g_nextSlot;
        g_slots[slot].name   = name;
        g_slots[slot].target = reinterpret_cast<void*>(target);

        // MinHook: create the hook (does not patch yet)
        MH_STATUS st = MH_CreateHook(
            reinterpret_cast<void*>(target),
            reinterpret_cast<void*>(g_detourTable[slot]),
            &g_slots[slot].original);

        if (st != MH_OK && st != MH_ERROR_ALREADY_CREATED) {
            Logger::TimestampLogf("[CallLogger] FAIL MH_CreateHook '%s': %s",
                         name, MH_StatusToString(st));
            continue;
        }

        // MinHook: enable (patches the original function)
        st = MH_EnableHook(reinterpret_cast<void*>(target));
        if (st != MH_OK && st != MH_ERROR_ENABLED) {
            Logger::TimestampLogf("[CallLogger] FAIL MH_EnableHook '%s': %s",
                         name, MH_StatusToString(st));
            continue;
        }

        g_slots[slot].active = true;
        g_nextSlot++;
        installed++;

        Logger::TimestampLogf("[CallLogger] Hooked: %s @ 0x%08X (slot %d)",
                     name, static_cast<unsigned>(rva), slot);
    }

    Logger::TimestampLogf("[CallLogger] RegisterAll complete — %d hook(s) installed", installed);
    return installed;
}

void UnregisterAll() {
    for (int i = 0; i < g_nextSlot; ++i) {
        if (g_slots[i].active && g_slots[i].target)
            MH_DisableHook(g_slots[i].target);
        g_slots[i] = HookSlot{};
    }
    g_nextSlot = 0;
}

} // namespace CallLogger

#endif
