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

} // namespace Randomizer
