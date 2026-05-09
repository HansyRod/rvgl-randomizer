#pragma once

#include <string>
#include <vector>
#include <optional>
#include <fstream>
#include <nlohmann/json.hpp>
#include "RVGLStructs.h"

using json = nlohmann::json;

namespace Randomizer {

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
        p.load_extra_cars = j.value("load_extra_cars", false);
        p.load_extra_tracks = j.value("load_extra_tracks", false);
        p.load_extra_cups = j.value("load_extra_cups", false);
        p.is_stock_cars = j.value("is_stock_cars", false);
        p.is_stock_tracks = j.value("is_stock_tracks", false);
    }

    // Represents an entry in the "cars" array
    struct RandomizedCar {
        std::string folder;
        CarRating rating;
        Obtain obtain;
        bool selectable_player;
        bool selectable_cpu;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RandomizedCar, folder, rating, obtain, selectable_player, selectable_cpu)

    // Represents an entry in the "tracks" array
    struct RandomizedTrack {
        std::string folder;
        int difficulty;
        Obtain obtain;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RandomizedTrack, folder, difficulty, obtain)

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
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RandomizedCup, name, difficulty, obtainCondition, numCars, numTries, perRaceRequiredPlace, overallRequiredPlace, carsPerClass, pointsTable, stages)

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