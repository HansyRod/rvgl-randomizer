#pragma once

#include "ConfigData.h"
#include <optional>
#include <string>


namespace Randomizer {

std::optional<ConfigData> LoadConfiguration();

} // namespace Randomizer