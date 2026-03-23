#pragma once
#include <vector>
#include <string>
#include "Carbox.h"

namespace Randomizer {

    void GenerateAndSaveCarboxAtlas(const std::string& outputPath, const std::vector<CarboxSource>& cars);

}