#pragma once

#include <cstddef>
#include <string>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace Platform {

#if defined(_WIN32)
using ModuleHandle = HMODULE;
#else
using ModuleHandle = void*;
#endif

std::string GetModulePath(ModuleHandle module);
std::string GetCurrentDirectoryString();
std::vector<std::string> GetProcessArguments();

bool FileExists(const std::string& path);
bool UnsetEnv(const char* name);
void DebugLog(const char* message);

int CaseInsensitiveCompare(const char* left, const char* right);
int CaseInsensitiveNCompare(const char* left, const char* right, std::size_t count);
void CopyTruncated(char* dest, std::size_t destSize, const char* src);
bool WriteProtectedMemory(void* dest, const void* src, std::size_t size);

} // namespace Platform
