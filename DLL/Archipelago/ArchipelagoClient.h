#pragma once

#include <cstdint>
#include <string>

namespace Randomizer {

struct ArchipelagoConnectionConfig {
    bool enabled = false;
    std::string serverUri;
    std::string slotName;
    std::string password;
    std::string gameName = "RVGL";
    std::string uuid;
};

void ConfigureArchipelagoClient(const ArchipelagoConnectionConfig& config);
void StartArchipelagoClient();
void StopArchipelagoClient();
void PumpArchipelagoClient();
void QueueArchipelagoEventKey(const std::string& eventKey);
void QueueArchipelagoLocationCheck(int64_t locationId);
bool IsArchipelagoClientEnabled();

} // namespace Randomizer
