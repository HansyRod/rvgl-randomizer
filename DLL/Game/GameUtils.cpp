

#include "GameUtils.h"
#include "RVGLStructs.h"
#include <cstring>
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm> // Required for std::clamp

namespace Randomizer {

static const char* stockCars[28] = {
  "rc",    "mite",    "phat",   "moss",
  "mud",   "beatall", "volken", "tc6",
  "dino",  "candy",   "gencar", "tc4",
  "mouse", "flag",    "tc2",    "r5",
  "tc5",   "sgt",     "tc3",    "adeon",
  "fone",  "tc1",     "rotor",  "cougar",
  "sugo",  "toyeca",  "amw",    "panga"
};

std::string GetCarFolderName(const std::string& path) {
  if (_strnicmp(path.c_str(), "cars/", 5) == 0 ||
      _strnicmp(path.c_str(), "cars\\", 5) == 0) {
    return path.substr(5);
  }
  return path;
}

std::string GetCarFolderName(const CarInfo& car) {
  return GetCarFolderName(std::string(car.internalName, strnlen_s(car.internalName, sizeof(car.internalName))));
}

bool IsSameCarFolder(const std::string& left, const char* right) {
  if (right == nullptr) {
    return false;
  }

  const std::string normalizedLeft = GetCarFolderName(left);
  const std::string normalizedRight = GetCarFolderName(right);
  return _stricmp(normalizedLeft.c_str(), normalizedRight.c_str()) == 0;
}

bool IsStockCar(const std::string internalName) {

  for (int i = 0; i < 28; ++i) {
    if (internalName == stockCars[i]) {
      return true;
    }
  }
  return false;
}


}