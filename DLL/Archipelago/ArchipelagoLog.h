#pragma once

namespace Randomizer {

// Writes a timestamped message to the base logger and updates the in-game log.
void LogArchipelagoMessage(const char* message);
void LogArchipelagoMessagef(const char* format, ...);
void DrawArchipelagoLog();

} // namespace Randomizer
