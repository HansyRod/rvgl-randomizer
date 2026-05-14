#include "Fob.h"
#include "Addresses.h"
#include "FileUtils.h"
#include "Logger.h"
#include "RVGLStructs.h"
#include <array>
#include <fstream>
#include <string>

// ============================================================================
// Fob.cpp
//
// Hooks for modifying FOB file loading
// ============================================================================

namespace {

constexpr int kFobObjectPickup = 30;
constexpr int kFobObjectStar = 50;
constexpr int kStarSubtypeGlobalWeapon = 0;
constexpr int kStarSubtypePracticeStar = 1;

struct FobRecord {
    int objectId = 0;
    std::array<int, 4> subinfos{};
    std::array<float, 3> position{};
    std::array<float, 6> rotation{};
};

static_assert(sizeof(int) == 4, "FOB parser expects 32-bit int fields");
static_assert(sizeof(float) == 4, "FOB parser expects 32-bit float fields");

bool ReadFobRecord(std::ifstream& file, FobRecord& record) {
    file.read(reinterpret_cast<char*>(&record.objectId), sizeof(record.objectId));
    file.read(reinterpret_cast<char*>(record.subinfos.data()), sizeof(int) * record.subinfos.size());
    file.read(reinterpret_cast<char*>(record.position.data()), sizeof(float) * record.position.size());
    file.read(reinterpret_cast<char*>(record.rotation.data()), sizeof(float) * record.rotation.size());
    return file.good();
}

std::array<float, 9> BuildRotationMatrix(const FobRecord& record) {
    const float forwardX = record.rotation[0];
    const float forwardY = record.rotation[1];
    const float forwardZ = record.rotation[2];
    const float upX = record.rotation[3];
    const float upY = record.rotation[4];
    const float upZ = record.rotation[5];

    return {
        upZ * forwardY - upY * forwardZ,
        forwardZ * upX - upZ * forwardX,
        upY * forwardX - upX * forwardY,
        forwardX,
        forwardY,
        forwardZ,
        upX,
        upY,
        upZ,
    };
}

bool TryFindPracticeStarCandidate(const char* fobFilePath, FobRecord& candidate, bool& hasPracticeStar) {
    hasPracticeStar = false;

    if (fobFilePath == nullptr || fobFilePath[0] == '\0') {
        Logger::TimestampLog("[Fob] Could not inspect FOB: empty path.");
        return false;
    }

    const std::string resolvedPath = Randomizer::GetAbsoluteFilePath(fobFilePath);
    std::ifstream file(resolvedPath, std::ios::binary);
    if (!file.is_open()) {
        Logger::TimestampLogf("[Fob] Could not open FOB for inspection: %s", resolvedPath.c_str());
        return false;
    }

    int objectCount = 0;
    file.read(reinterpret_cast<char*>(&objectCount), sizeof(objectCount));
    if (!file.good() || objectCount < 0) {
        Logger::TimestampLogf("[Fob] Invalid FOB header: %s", resolvedPath.c_str());
        return false;
    }

    bool hasGlobalStarCandidate = false;
    bool hasPickupCandidate = false;
    FobRecord pickupCandidate{};

    for (int i = 0; i < objectCount; ++i) {
        FobRecord record{};
        if (!ReadFobRecord(file, record)) {
            Logger::TimestampLogf("[Fob] Stopped reading malformed FOB after %d object(s): %s", i, resolvedPath.c_str());
            break;
        }

        if (record.objectId == kFobObjectStar && record.subinfos[0] == kStarSubtypePracticeStar) {
            hasPracticeStar = true;
            return true;
        }

        if (!hasGlobalStarCandidate &&
            record.objectId == kFobObjectStar &&
            record.subinfos[0] == kStarSubtypeGlobalWeapon) {
            candidate = record;
            hasGlobalStarCandidate = true;
        }

        if (!hasPickupCandidate && record.objectId == kFobObjectPickup) {
            pickupCandidate = record;
            hasPickupCandidate = true;
        }
    }

    if (hasGlobalStarCandidate) {
        return true;
    }

    if (hasPickupCandidate) {
        candidate = pickupCandidate;
        return true;
    }

    return true;
}

} // anonymous namespace

namespace Randomizer {

// ----------------------------------------------------------------------------
// Original function pointers
// MinHook writes the trampoline addresses into these during InstallAll().
// ----------------------------------------------------------------------------
FnLoadObjectsFromFob        Orig_LoadObjectsFromFob        = nullptr;
FnCreateObjectFromFob       Orig_CreateObjectFromFob       = reinterpret_cast<FnCreateObjectFromFob>(AbsFromRva(RVA_CREATE_OBJECT_FROM_FOB));

void Hook_LoadObjectsFromFob(char* fobFilePath) {

    Orig_LoadObjectsFromFob(fobFilePath);

    GameMode* gameMode = reinterpret_cast<GameMode*>(AbsFromRva(RVA_GAME_MODE));
    if (gameMode == nullptr || *gameMode != MODE_PRACTICE) {
        return;
    }

    FobRecord candidate{};
    bool hasPracticeStar = false;
    if (!TryFindPracticeStarCandidate(fobFilePath, candidate, hasPracticeStar)) {
        return;
    }

    if (hasPracticeStar) {
        Logger::TimestampLogf("[Fob] Practice star already present: %s", fobFilePath);
        return;
    }

    if (candidate.objectId == 0) {
        Logger::TimestampLogf("[Fob] No practice star candidate found: %s", fobFilePath);
        return;
    }

    auto rotationMatrix = BuildRotationMatrix(candidate);
    std::array<int, 4> practiceStarSubinfos{ kStarSubtypePracticeStar, 0, 0, 0 };
    void* createdObject = Orig_CreateObjectFromFob(
        candidate.position.data(),
        rotationMatrix.data(),
        kFobObjectStar,
        practiceStarSubinfos.data()
    );

    if (createdObject == nullptr) {
        Logger::TimestampLogf("[Fob] Failed to add generated practice star: %s", fobFilePath);
        return;
    }

    Logger::TimestampLogf(
        "[Fob] Added generated practice star at %.2f, %.2f, %.2f from object %d: %s",
        candidate.position[0],
        candidate.position[1],
        candidate.position[2],
        candidate.objectId,
        fobFilePath
    );

}

} // namespace Randomizer
