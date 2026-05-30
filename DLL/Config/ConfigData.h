#pragma once

#include <string>
#include <vector>
#include <optional>
#include <fstream>
#include <nlohmann/json.hpp>
#include "RVGLStructs.h"
#include "CustomUnlocks.h"

using json = nlohmann::json;

namespace Randomizer {

    inline void to_json(json& j, const CustomUnlockCondition& p) {
        j = json{
            {"trackFolder", p.trackFolders},
            {"requiredCount", p.requiredCount},
            {"archipelagoItem", p.archipelagoItem}
        };
    }

    inline void from_json(const json& j, CustomUnlockCondition& p) {
        p.trackFolders.clear();
        if (j.contains("trackFolder") && !j.at("trackFolder").is_null()) {
            p.trackFolders = j.at("trackFolder").get<std::vector<std::string>>();
        }
        if (j.contains("requiredCount") && !j.at("requiredCount").is_null()) {
            p.requiredCount = j.at("requiredCount").get<int>();
        }
        if (j.contains("archipelagoItem") && !j.at("archipelagoItem").is_null()) {
            p.archipelagoItem = j.at("archipelagoItem").get<std::string>();
        }
    }

    // Represents the "metadata" block
    struct ConfigMetadata {
        std::string seed;
        std::string version;
        std::optional<std::string> profileName;
    };

    inline void to_json(json& j, const ConfigMetadata& p) {
        j = json{{"seed", p.seed}, {"version", p.version}};
        if (p.profileName.has_value()) {
            j["profileName"] = p.profileName.value();
        }
    }

    inline void from_json(const json& j, ConfigMetadata& p) {
        j.at("seed").get_to(p.seed);
        j.at("version").get_to(p.version);
        if (j.contains("profileName") && !j.at("profileName").is_null()) {
            p.profileName = j.at("profileName").get<std::string>();
        } else {
            p.profileName = std::nullopt;
        }
    }

    // Represents the "global_options" block
    struct ConfigGlobalOptions {
        bool load_extra_cars = false;
        bool load_extra_tracks = false;
        bool load_extra_cups = false;
        bool is_stock_cars = false;
        bool is_stock_tracks = false;
    };

    inline bool GetOptionalBool(const json& j, const char* primaryKey, const char* fallbackKey, bool defaultValue) {
        if (j.contains(primaryKey) && !j.at(primaryKey).is_null()) {
            return j.at(primaryKey).get<bool>();
        }

        if (j.contains(fallbackKey) && !j.at(fallbackKey).is_null()) {
            return j.at(fallbackKey).get<bool>();
        }

        return defaultValue;
    }

    inline void to_json(json& j, const ConfigGlobalOptions& p) {
        j = json{
            {"load_extra_cars", p.load_extra_cars},
            {"load_extra_tracks", p.load_extra_tracks},
            {"load_extra_cups", p.load_extra_cups},
            {"is_stock_cars", p.is_stock_cars},
            {"is_stock_tracks", p.is_stock_tracks}
        };
    }

    inline void from_json(const json& j, ConfigGlobalOptions& p) {
        p.load_extra_cars = GetOptionalBool(j, "load_extra_cars", "loadExtraCars", false);
        p.load_extra_tracks = GetOptionalBool(j, "load_extra_tracks", "loadExtraTracks", false);
        p.load_extra_cups = GetOptionalBool(j, "load_extra_cups", "loadExtraCups", false);
        p.is_stock_cars = GetOptionalBool(j, "is_stock_cars", "isStockCars", false);
        p.is_stock_tracks = GetOptionalBool(j, "is_stock_tracks", "isStockTracks", false);
    }

    // Represents an entry in the "cars" array
    struct RandomizedCar {
        std::string folder;
        CarRating rating;
        Obtain obtain;
        bool selectable_player;
        bool selectable_cpu;
        std::optional<CustomUnlockCondition> customUnlock;
    };

    inline void to_json(json& j, const RandomizedCar& p) {
        j = json{
            {"folder", p.folder},
            {"rating", p.rating},
            {"obtain", p.obtain},
            {"selectable_player", p.selectable_player},
            {"selectable_cpu", p.selectable_cpu}
        };
        if (p.customUnlock.has_value()) {
            j["customUnlock"] = p.customUnlock.value();
        }
    }

    inline void from_json(const json& j, RandomizedCar& p) {
        j.at("folder").get_to(p.folder);
        j.at("rating").get_to(p.rating);
        j.at("obtain").get_to(p.obtain);
        j.at("selectable_player").get_to(p.selectable_player);
        j.at("selectable_cpu").get_to(p.selectable_cpu);
        if (j.contains("customUnlock") && !j.at("customUnlock").is_null()) {
            p.customUnlock = j.at("customUnlock").get<CustomUnlockCondition>();
        } else {
            p.customUnlock = std::nullopt;
        }
    }

    // Represents an entry in the "tracks" array
    struct RandomizedTrack {
        std::string folder;
        int difficulty;
        Obtain obtain;
        std::optional<CustomUnlockCondition> customUnlock;
    };

    inline void to_json(json& j, const RandomizedTrack& p) {
        j = json{
            {"folder", p.folder},
            {"difficulty", p.difficulty},
            {"obtain", p.obtain}
        };
        if (p.customUnlock.has_value()) {
            j["customUnlock"] = p.customUnlock.value();
        }
    }

    inline void from_json(const json& j, RandomizedTrack& p) {
        j.at("folder").get_to(p.folder);
        j.at("difficulty").get_to(p.difficulty);
        j.at("obtain").get_to(p.obtain);
        if (j.contains("customUnlock") && !j.at("customUnlock").is_null()) {
            p.customUnlock = j.at("customUnlock").get<CustomUnlockCondition>();
        } else {
            p.customUnlock = std::nullopt;
        }
    }

    // Represents an entry in the "stages" array within a cup
    struct RandomizedCupStage {
        std::string trackFolder;
        int numLaps;
        bool isReverse;
        bool isMirror;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RandomizedCupStage, trackFolder, numLaps, isReverse, isMirror)

    struct RandomizedCup {
        std::string name;
        int difficulty;
        Obtain obtainCondition;
        int numCars;
        int numTries;
        int perRaceRequiredPlace;
        int overallRequiredPlace;
        std::vector<int> carsPerClass; // Max number of AI allowed from each class (Rookie, Amateur, etc.)
        std::vector<int> pointsTable; // Points for each position (1st to 16th)
        std::vector<RandomizedCupStage> stages;
        std::optional<CustomUnlockCondition> customUnlock;
    };

    inline void to_json(json& j, const RandomizedCup& p) {
        j = json{
            {"name", p.name},
            {"difficulty", p.difficulty},
            {"obtainCondition", p.obtainCondition},
            {"numCars", p.numCars},
            {"numTries", p.numTries},
            {"perRaceRequiredPlace", p.perRaceRequiredPlace},
            {"overallRequiredPlace", p.overallRequiredPlace},
            {"carsPerClass", p.carsPerClass},
            {"pointsTable", p.pointsTable},
            {"stages", p.stages}
        };
        if (p.customUnlock.has_value()) {
            j["customUnlock"] = p.customUnlock.value();
        }
    }

    inline void from_json(const json& j, RandomizedCup& p) {
        j.at("name").get_to(p.name);
        j.at("difficulty").get_to(p.difficulty);
        j.at("obtainCondition").get_to(p.obtainCondition);
        j.at("numCars").get_to(p.numCars);
        j.at("numTries").get_to(p.numTries);
        j.at("perRaceRequiredPlace").get_to(p.perRaceRequiredPlace);
        j.at("overallRequiredPlace").get_to(p.overallRequiredPlace);
        j.at("carsPerClass").get_to(p.carsPerClass);
        j.at("pointsTable").get_to(p.pointsTable);
        j.at("stages").get_to(p.stages);
        if (j.contains("customUnlock") && !j.at("customUnlock").is_null()) {
            p.customUnlock = j.at("customUnlock").get<CustomUnlockCondition>();
        } else {
            p.customUnlock = std::nullopt;
        }
    }

    // The root structure containing all randomizer data
    struct ConfigData {
        ConfigMetadata metadata;
        ConfigGlobalOptions global_options;
        std::vector<RandomizedCar> stockCars;
        std::vector<RandomizedCar> dcCars;
        std::vector<RandomizedTrack> tracks;
        std::vector<RandomizedCup> cups;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ConfigData, metadata, global_options, stockCars, dcCars, tracks, cups)

} // namespace Randomizer
