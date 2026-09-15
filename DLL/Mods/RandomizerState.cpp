#include "RandomizerState.h"

namespace Randomizer {

RandomizerContext& GetRandomizerContext() {
    static RandomizerContext context;
    return context;
}

ConfigData* GetActiveConfig() {
    auto& activeConfig = GetRandomizerContext().config.activeConfig;
    return activeConfig.has_value() ? &activeConfig.value() : nullptr;
}

bool IsThirtyCarModeEnabled() {
    const ConfigData* config = GetActiveConfig();
    return config != nullptr && config->global_options.enable_30_car_mode;
}

bool IsKnockoutModeEnabled() {
    const ConfigData* config = GetActiveConfig();
    return config != nullptr && config->global_options.enable_knockout_mode;
}

} // namespace Randomizer
