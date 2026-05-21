#include "Logger.h"
#include "ConfigManager.h"
#include "RandomizerState.h"

// Move startup/config work into RandomizerInit.h/.cpp:
// Keep Initialize() here.
// Load config once, normalize any startup-only derived flags, and leave car/track-specific runtime preparation to their own modules.
// dllmain.cpp should include only this header.

namespace Randomizer {

void Initialize() {

    RandomizerContext& ctx = GetRandomizerContext();

    Logger::TimestampLogf("[Randomizer] Initializing Randomizer module...");

    // 1. Attempt to load the configuration file
    // File path is provided by environment variable passed from the Rust module

    ctx.config.activeConfig = LoadConfiguration();
    ConfigData* config = GetActiveConfig();

    // 2. Verify successful load and output test data
    if (config != nullptr) {
        Logger::TimestampLogf("[Randomizer] Successfully loaded configuration!");
        Logger::TimestampLogf("[Randomizer] Seed: %s", config->metadata.seed.c_str());
        Logger::TimestampLogf("[Randomizer] Parsed %zu stock cars, %zu DC cars, and %zu tracks.", 
            config->stockCars.size(), 
            config->dcCars.size(), 
            config->tracks.size());
        
        // Example of accessing nested data
        if (!config->stockCars.empty()) {
            Logger::TimestampLogf("[Randomizer] First stock car folder: %s (Rating: %d)", 
                config->stockCars[0].folder.c_str(), 
                config->stockCars[0].rating);
        }
        
        const char** patchedPtrs = ctx.carState.patchedPtrs;

        // Init patchedPtrs based on config
        for (size_t i = 0; i < config->stockCars.size() && i <= 27; ++i) {
            std::string carPath = config->stockCars[i].folder;
            if (carPath.rfind("cars/", 0) != 0) {
                carPath = "cars/" + carPath; // Ensure the path has the correct prefix
            }
            config->stockCars[i].folder = carPath; // Update the folder in the config struct
            patchedPtrs[i] = config->stockCars[i].folder.c_str();
        }
        for (size_t i = 0; i < config->dcCars.size() && i <= 13; ++i) {
            std::string carPath = config->dcCars[i].folder;
            if (carPath.rfind("cars/", 0) != 0) {
                carPath = "cars/" + carPath; // Ensure the path has the correct prefix
            }
            config->dcCars[i].folder = carPath; // Update the folder in the config struct
            patchedPtrs[35 + i] = config->dcCars[i].folder.c_str();
        }
        // Apply cupDC flag
        if (config->global_options.is_stock_tracks == true) {
            ctx.config.useCupDC = false;
        }
    } else {
        Logger::TimestampLogf("[Randomizer] Failed to load or parse configuration. Mod will remain inactive.");
        // Depending on architecture, you might want to disable further hooks here
    }
}


}