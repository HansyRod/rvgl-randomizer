#pragma once

namespace Randomizer {

// Writes a timestamped message to the base logger and updates the in-game log.
// Supply unprefixed text; the [Archipelago] tag is added only to the base log entry.
// Use for server messages, connection status, and errors relevant to players.
// Internal diagnostics use Logger::TimestampLog[f] with an [Archipelago] tag instead.
void LogArchipelagoMessage(const char* message);
void LogArchipelagoMessagef(const char* format, ...);
void DrawArchipelagoLog();

} // namespace Randomizer
