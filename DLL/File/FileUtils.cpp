#include "FileUtils.h"
#include "Platform.h"
#include <filesystem>
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

namespace {

std::string NormalizeSeparators(std::string path) {
#if defined(_WIN32)
    constexpr char separator = '\\';
#else
    constexpr char separator = '/';
#endif
    std::replace(path.begin(), path.end(), '\\', separator);
    std::replace(path.begin(), path.end(), '/', separator);
    return path;
}

std::string FindPacklistArgument() {
    const std::vector<std::string> args = Platform::GetProcessArguments();
    for (std::size_t i = 0; i + 1 < args.size(); ++i) {
        if (args[i] == "-packlist") {
            return args[i + 1];
        }
    }

    return "";
}

} // anonymous namespace

void InitializePacklistCache() {
    if (s_packlistInitialized) return;
    s_packlistInitialized = true;

    const std::string packlistName = FindPacklistArgument();

    // If no -packlist argument exists, it is a classic install. Abort pack initialization.
    if (packlistName.empty()) {
        return; 
    }

    // We are in a pack environment. CWD is <root>/packs/<platform>/
    const std::filesystem::path cwd = Platform::GetCurrentDirectoryString();
    const std::filesystem::path packsDir = cwd.parent_path();

    if (!packsDir.empty()) {
        s_packsDir = packsDir.string();

        const std::filesystem::path packlistPath = packsDir / (packlistName + ".txt");
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
            Logger::TimestampLogf("[Randomizer] WARNING: -packlist provided, but file %s could not be opened.", packlistPath.string().c_str());
        }
    }
}

std::string GetAbsoluteFilePath(const std::string& relativePath) {
    const std::string searchPath = NormalizeSeparators(relativePath);
    const std::filesystem::path cwd = Platform::GetCurrentDirectoryString();

    // 1. Classic install check (assumes CWD is the root directory)
    const std::filesystem::path classicCandidate = cwd / searchPath;
    if (Platform::FileExists(classicCandidate.string())) {
        return classicCandidate.string();
    }

    // 2. Launcher / Pack install check
    InitializePacklistCache(); // Only executes once
    
    if (!s_packsDir.empty()) {
        const std::filesystem::path packsDir = s_packsDir;

        // Search in reverse order to respect pack priority (later packs overwrite earlier ones)
        for (auto it = s_activePacks.rbegin(); it != s_activePacks.rend(); ++it) {
            const std::filesystem::path candidate = packsDir / *it / searchPath;
            if (Platform::FileExists(candidate.string())) {
                return candidate.string();
            }
        }
    }

    // Return the original relative path if we couldn't find an override.
    // This allows standard fallback routines (like stbi_load or the game engine) to try resolving it themselves.
    return relativePath; 
}

}