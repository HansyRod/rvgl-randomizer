

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

bool IsStockCar(const std::string internalName) {

  for (int i = 0; i < 28; ++i) {
    if (internalName == stockCars[i]) {
      return true;
    }
  }
  return false;
}


}