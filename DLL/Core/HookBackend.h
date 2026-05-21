#pragma once

namespace HookBackend {

bool Initialize();
bool CreateHook(void* target, void* detour, void** original, const char** error);
bool EnableHook(void* target, const char** error);
bool DisableHook(void* target, const char** error);
void Uninitialize();

} // namespace HookBackend
