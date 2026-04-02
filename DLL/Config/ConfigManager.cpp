#include "ConfigManager.h"
#include "ConfigData.h"
#include <iostream>
#include <fstream>

namespace Randomizer {

    // Attempts to load and parse the JSON file into the C++ structs.
    // Returns std::nullopt if the file cannot be read or parsing fails.
    std::optional<ConfigData> LoadConfiguration(const std::string& filepath) {
        std::ifstream fileStream(filepath);
        
        if (!fileStream.is_open()) {
            // TODO: Integrate with existing Logger.cpp
            std::cerr << "[Randomizer] Error: Could not open config file: " << filepath << std::endl;
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