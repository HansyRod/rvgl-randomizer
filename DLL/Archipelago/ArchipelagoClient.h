#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace Randomizer {

struct ArchipelagoConfig;

struct ArchipelagoConnectionConfig {
    bool enabled = false;
    std::string serverUri;
    std::string slotName;
    std::string password;
    std::string gameName = "RVGL";
    std::string uuid;
};

void ConfigureArchipelagoClient(const ArchipelagoConnectionConfig& config);
// Supplied by profile loading; connecting waits until the profile UUID is available.
void SetArchipelagoClientUUID(const std::string& uuid);
// Passing nullptr clears the mappings and pending checks for non-AP configurations.
void InitializeArchipelagoMappings(const ArchipelagoConfig* config);
// Unknown IDs return nullopt; filler resolves to an empty string.
std::optional<std::string> GetArchipelagoItemName(int itemId);
void StartArchipelagoClient();
void StopArchipelagoClient();
void PumpArchipelagoClient();
void QueueArchipelagoEventKey(const std::string& eventKey);
void QueueArchipelagoLocationCheck(int64_t locationId);
bool IsArchipelagoClientEnabled();

} // namespace Randomizer
