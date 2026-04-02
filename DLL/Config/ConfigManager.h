#pragma once

#include <string>
#include <optional>
#include "ConfigData.h"

namespace Randomizer {

    std::optional<ConfigData> LoadConfiguration(const std::string& filepath);

} // namespace Randomizer