#include "RVGLAddresses.h"

#include <cstring>
#include <string>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <link.h>
#endif

namespace RVGL {

namespace {

uintptr_t g_moduleBase = 0;

#if !defined(_WIN32)
int FindRvglModule(struct dl_phdr_info* info, size_t, void*) {
    if (info == nullptr || info->dlpi_name == nullptr) {
        return 0;
    }

    const std::string name = info->dlpi_name;
    const bool isMainExecutable = name.empty();
    const bool looksLikeRvgl = name.find("rvgl.64") != std::string::npos || name.find("/rvgl") != std::string::npos;

    if (isMainExecutable || looksLikeRvgl) {
        uintptr_t lowestLoadAddress = UINTPTR_MAX;
        for (int i = 0; i < info->dlpi_phnum; ++i) {
            const ElfW(Phdr)& header = info->dlpi_phdr[i];
            if (header.p_type == PT_LOAD && header.p_vaddr < lowestLoadAddress) {
                lowestLoadAddress = static_cast<uintptr_t>(header.p_vaddr);
            }
        }

        if (lowestLoadAddress == UINTPTR_MAX) {
            lowestLoadAddress = 0;
        }

        g_moduleBase = static_cast<uintptr_t>(info->dlpi_addr) + lowestLoadAddress;
        return 1;
    }

    return 0;
}
#endif

} // anonymous namespace

uintptr_t ModuleBase() {
    if (g_moduleBase != 0) {
        return g_moduleBase;
    }

#if defined(_WIN32)
    g_moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandleA("rvgl.exe"));
#else
    dl_iterate_phdr(FindRvglModule, nullptr);
#endif

    return g_moduleBase;
}

uintptr_t AbsFromRva(uint32_t rva) {
    const uintptr_t base = ModuleBase();
    return base != 0 ? base + rva : 0;
}

uint8_t* GetTrackProgressCache() {
#if defined(__linux__)
    return reinterpret_cast<uint8_t*>(AbsFromRva(RVA_TRACK_PROGRESS_CACHE_PTR));
#else
    return *reinterpret_cast<uint8_t**>(AbsFromRva(RVA_TRACK_PROGRESS_CACHE_PTR));
#endif
}

} // namespace RVGL
