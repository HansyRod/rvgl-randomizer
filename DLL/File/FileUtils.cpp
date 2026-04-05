#include <windows.h>
#include <shellapi.h>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include "Logger.h"

namespace Randomizer {

// Static cache to avoid parsing the packlist text file on every single texture load
static std::vector<std::string> s_activePacks;
static std::string s_packsDir = "";
static bool s_packlistInitialized = false;

void InitializePacklistCache() {
    if (s_packlistInitialized) return;
    s_packlistInitialized = true;

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::string packlistName = "";
    
    // 1. Explicitly check for the -packlist argument first
    if (argv) {
        for (int i = 0; i < argc; i++) {
            if (std::wstring(argv[i]) == L"-packlist" && (i + 1) < argc) {
                std::wstring nextArg = argv[i + 1];
                int size_needed = WideCharToMultiByte(CP_UTF8, 0, &nextArg[0], (int)nextArg.size(), NULL, 0, NULL, NULL);
                std::string utf8Str(size_needed, 0);
                WideCharToMultiByte(CP_UTF8, 0, &nextArg[0], (int)nextArg.size(), &utf8Str[0], size_needed, NULL, NULL);
                packlistName = utf8Str;
                break;
            }
        }
        LocalFree(argv);
    }

    // 2. If no -packlist argument exists, it is a classic install. Abort pack initialization.
    if (packlistName.empty()) {
        return; 
    }

    // 3. We are in a pack environment. CWD is <root>/packs/<platform>/
    char cwd[MAX_PATH]{};
    GetCurrentDirectoryA(MAX_PATH, cwd);
    std::string cwdStr = cwd;
    
    size_t slash = cwdStr.find_last_of("\\/");
    if (slash != std::string::npos) {
        s_packsDir = cwdStr.substr(0, slash); // Strip <platform> to get the packs root
        
        std::string packlistPath = s_packsDir + "\\" + packlistName + ".txt";
        std::ifstream infile(packlistPath);

        if (infile.is_open()) {
            std::string line;
            while (std::getline(infile, line)) {
                size_t commentPos = line.find(';');
                if (commentPos != std::string::npos) line = line.substr(0, commentPos);
                size_t asteriskPos = line.find('*');
                if (asteriskPos != std::string::npos) line = line.substr(0, asteriskPos);

                size_t first = line.find_first_not_of(" \t\r\n\"");
                if (first == std::string::npos) continue; 
                size_t last = line.find_last_not_of(" \t\r\n\"");
                line = line.substr(first, (last - first + 1));

                if (!line.empty()) {
                    if (line == "default") {
                        s_activePacks.push_back(".."); 
                    } else {
                        s_activePacks.push_back(line);
                    }
                }
            }
        } else {
            Logger::TimestampLogf("[Randomizer] WARNING: -packlist provided, but file %s could not be opened.", packlistPath.c_str());
        }
    }
}

std::string GetAbsoluteFilePath(const std::string& relativePath) {
    // Normalize forward slashes to backslashes for Windows API compatibility
    std::string searchPath = relativePath;
    std::replace(searchPath.begin(), searchPath.end(), '/', '\\');

    char cwd[MAX_PATH]{};
    GetCurrentDirectoryA(MAX_PATH, cwd);
    std::string cwdStr = cwd;

    // 1. Classic install check (assumes CWD is the root directory)
    std::string classicCandidate = cwdStr + "\\" + searchPath;
    if (GetFileAttributesA(classicCandidate.c_str()) != INVALID_FILE_ATTRIBUTES) {
        return classicCandidate;
    }

    // 2. Launcher / Pack install check
    InitializePacklistCache(); // Only executes once
    
    if (!s_packsDir.empty()) {
        // Search in reverse order to respect pack priority (later packs overwrite earlier ones)
        for (auto it = s_activePacks.rbegin(); it != s_activePacks.rend(); ++it) {
            std::string candidate = s_packsDir + "\\" + *it + "\\" + searchPath;
            if (GetFileAttributesA(candidate.c_str()) != INVALID_FILE_ATTRIBUTES) {
                return candidate;
            }
        }
    }

    // Return the original relative path if we couldn't find an override.
    // This allows standard fallback routines (like stbi_load or the game engine) to try resolving it themselves.
    return relativePath; 
}

}