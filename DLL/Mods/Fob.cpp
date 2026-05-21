#include "Fob.h"
#include "RVGLAddresses.h"
#include "FileUtils.h"
#include "Logger.h"
#include "RVGLFunctions.h"
#include "RVGLStructs.h"
#include <array>
#include <fstream>
#include <random>
#include <string>
#include <vector>

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
constexpr int kCandidateSourceNone = 0;
constexpr int kCandidateSourcePickup = kFobObjectPickup;
constexpr int kCandidateSourceGlobalStar = kFobObjectStar;
constexpr int kCandidateSourceRouteNode = -1;
constexpr float kRouteNodeStarYLift = -30.0f;

std::mt19937& Rng() {
    static std::mt19937 rng{ std::random_device{}() };
    return rng;
}

template <typename T>
const T& PickRandom(const std::vector<T>& items) {
    std::uniform_int_distribution<size_t> dist(0, items.size() - 1);
    return items[dist(Rng())];
}

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

std::string ResolveGameFilePath(const std::string& filePath) {
    std::ifstream directFile(filePath, std::ios::binary);
    if (directFile.is_open()) {
        return filePath;
    }

    return Randomizer::GetAbsoluteFilePath(filePath);
}

std::string MakeSiblingPanPath(const char* fobFilePath) {
    if (fobFilePath == nullptr) {
        return {};
    }

    std::string panPath = fobFilePath;
    const size_t lastSlash = panPath.find_last_of("\\/");
    const size_t lastDot = panPath.find_last_of('.');
    if (lastDot != std::string::npos && (lastSlash == std::string::npos || lastSlash < lastDot)) {
        panPath.replace(lastDot, std::string::npos, ".pan");
    }
    else {
        panPath += ".pan";
    }
    return panPath;
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

FobRecord MakeRouteNodeCandidate(const std::array<float, 3>& position) {
    FobRecord candidate{};
    candidate.objectId = kCandidateSourceRouteNode;
    candidate.position = position;
    candidate.position[1] += kRouteNodeStarYLift;
    candidate.rotation = { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f };
    return candidate;
}

bool TryReadRandomRouteNodeCandidate(const char* fobFilePath, FobRecord& candidate) {
    const std::string panPath = MakeSiblingPanPath(fobFilePath);
    if (panPath.empty()) {
        return false;
    }

    const std::string resolvedPath = ResolveGameFilePath(panPath);
    std::ifstream file(resolvedPath, std::ios::binary);
    if (!file.is_open()) {
        Logger::TimestampLogf("[Fob] Could not open PAN route nodes for fallback: %s", resolvedPath.c_str());
        return false;
    }

    int nodeCount = 0;
    int startNode = 0;
    float totalDistance = 0.0f;
    file.read(reinterpret_cast<char*>(&nodeCount), sizeof(nodeCount));
    file.read(reinterpret_cast<char*>(&startNode), sizeof(startNode));
    file.read(reinterpret_cast<char*>(&totalDistance), sizeof(totalDistance));

    if (!file.good() || nodeCount <= 0) {
        Logger::TimestampLogf("[Fob] Invalid PAN header: %s", resolvedPath.c_str());
        return false;
    }

    std::vector<std::array<float, 3>> routeNodePositions;
    routeNodePositions.reserve(static_cast<size_t>(nodeCount));

    for (int i = 0; i < nodeCount; ++i) {
        std::array<float, 3> position{};
        float distanceToFinish = 0.0f;
        std::array<int, 4> previous{};
        std::array<int, 4> next{};

        file.read(reinterpret_cast<char*>(position.data()), sizeof(float) * position.size());
        file.read(reinterpret_cast<char*>(&distanceToFinish), sizeof(distanceToFinish));
        file.read(reinterpret_cast<char*>(previous.data()), sizeof(int) * previous.size());
        file.read(reinterpret_cast<char*>(next.data()), sizeof(int) * next.size());

        if (!file.good()) {
            Logger::TimestampLogf("[Fob] Stopped reading malformed PAN after %d node(s): %s", i, resolvedPath.c_str());
            break;
        }

        routeNodePositions.push_back(position);
    }

    if (routeNodePositions.empty()) {
        return false;
    }

    std::array<float, 3> chosenRouteNode = PickRandom(routeNodePositions);

    candidate = MakeRouteNodeCandidate(chosenRouteNode);
    Logger::TimestampLogf("[Fob] Using random PAN route node fallback from %zu node(s): %s", routeNodePositions.size(), resolvedPath.c_str());
    return true;
}

bool TryFindPracticeStarCandidate(const char* fobFilePath, FobRecord& candidate, bool& hasPracticeStar) {
    hasPracticeStar = false;

    if (fobFilePath == nullptr || fobFilePath[0] == '\0') {
        Logger::TimestampLog("[Fob] Could not inspect FOB: empty path.");
        return false;
    }

    const std::string resolvedPath = ResolveGameFilePath(fobFilePath);
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
    std::vector<FobRecord> pickupCandidates;

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

        if (record.objectId == kFobObjectPickup) {
            pickupCandidates.push_back(record);
        }
    }

    if (hasGlobalStarCandidate) {
        return true;
    }

    if (!pickupCandidates.empty()) {
        candidate = PickRandom(pickupCandidates);
        Logger::TimestampLogf("[Fob] Using random pickup fallback from %zu pickup(s): %s", pickupCandidates.size(), resolvedPath.c_str());
        return true;
    }

    TryReadRandomRouteNodeCandidate(fobFilePath, candidate);
    return true;
}

} // anonymous namespace

namespace Randomizer {

// ----------------------------------------------------------------------------
// Original function pointers
// MinHook writes the trampoline addresses into these during InstallAll().
// ----------------------------------------------------------------------------
FnLoadObjectsFromFob Orig_LoadObjectsFromFob = nullptr;

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

    if (candidate.objectId == kCandidateSourceNone) {
        Logger::TimestampLogf("[Fob] No practice star candidate found: %s", fobFilePath);
        return;
    }

    auto rotationMatrix = BuildRotationMatrix(candidate);
    std::array<int, 4> practiceStarSubinfos{ kStarSubtypePracticeStar, 0, 0, 0 };
    void* createdObject = RVGL_CreateObjectFromFob(
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
