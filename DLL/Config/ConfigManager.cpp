#include "ConfigManager.h"
#include "ConfigData.h"
#include "Logger.h"
#include <iostream>
#include <fstream>
#include <windows.h>

namespace Randomizer {

    // Attempts to load and parse the JSON file into the C++ structs.
    // Returns std::nullopt if the file cannot be read or parsing fails.
    std::optional<ConfigData> LoadConfiguration() {

        const char* envPath = std::getenv("RVGL_RANDOMIZER_CONFIG");
        
        if (!envPath || std::string(envPath).empty()) {
            // Handle the missing config safely. You might want to disable the hooks entirely 
            // if this is missing so the user can play vanilla normally.
            Logger::TimestampLogf("RVGL_RANDOMIZER_CONFIG environment variable not set. Hook disabled.");
            return std::nullopt;
        }

        std::string configFilePath = envPath;

        // Delete the environment variable to prevent polluting the process space
        SetEnvironmentVariableA("RVGL_RANDOMIZER_CONFIG", NULL);

        Logger::TimestampLogf(("Loading randomizer config from: " + configFilePath).c_str());

        std::ifstream fileStream(configFilePath);
        
        if (!fileStream.is_open()) {
            // TODO: Integrate with existing Logger.cpp
            std::cerr << "[Randomizer] Error: Could not open config file: " << configFilePath << std::endl;
            return std::nullopt;
        }

        try {
            json j;
            fileStream >> j;
            
            // The get<T>() function automatically uses the NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE macros
            // to map the JSON fields directly to the RandomizerData struct.
            ConfigData parsedData = j.get<ConfigData>();
            return parsedData;

        } catch (const json::exception& e) {
            std::cerr << "[Randomizer] JSON Parsing Error: " << e.what() << std::endl;
            return std::nullopt;
        }
    }

} // namespace Randomizer