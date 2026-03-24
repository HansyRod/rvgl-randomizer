#include "RVGLStructs.h"
#include "Carbox.h"
#include <vector>
#include <cstring>

namespace {

static const char* carboxGridNames[5][9] = {
  {
    "rc", "mite", "phat",
    "moss", "mud", "beatall",
    "volken", "tc6", "dino"
  },
  {
    "candy", "gencar", "tc4",
    "mouse", "flag", "tc2",
    "r5", "tc5", "sgt"
  },
  {
    "tc3", "adeon", "fone",
    "tc1", "rotor", "cougar",
    "sugo", "toyeca", "amw"
  },
  {
    "jg6rc", "tc12", "tc10",
    "tc8", "tc11", "tc9",
    "jg1jg7", "tc7", "jg3loco"
  },
  {
    "jg4snw35", "jg5purpxl", "jg2fulonx",
    "bigvolt", "bossvolt", "panga",
    "q", "", ""
  }
};

static const int carboxGridIndexes[5][9] = {
  {
    0, 1, 2,
    3, 4, 5,
    6, 7, 8
  },
  {
    9, 10, 11,
    12, 13, 14,
    15, 16, 17
  },
  {
    18, 19, 20,
    21, 22, 23,
    24, 25, 26
  },
  {
    37, 38, 39,
    40, 41, 42,
    43, 44, 45
  },
  {
    46, 47, 48,
    35, 36, 27,
    34, -1, -1
  }
};

}

namespace Randomizer {

CarboxSource GetCarboxSource(CarInfo* car) {
    CarboxSource src;
    std::string internalName = car->internalName;

    // 1. Custom Car (Has a standalone TCARBOX keyword defined)
    if (car->tcarboxFilename[0] != '\0') {
        src.filepath = car->tcarboxFilename;
        src.isFromGrid = false;
        src.sourceGridX = 0;
        src.sourceGridY = 0;
        return src;
    }

    // 2. Vanilla Car
    src.isFromGrid = true;

    // Search the grid for the car's internal name
    for (int gridIndex = 0; gridIndex < 5; ++gridIndex) {
        for (int cellIndex = 0; cellIndex < 9; ++cellIndex) {
            if (internalName == carboxGridNames[gridIndex][cellIndex]) {
                src.filepath = "cars/misc/carbox" + std::to_string(gridIndex + 1) + ".bmp";
                src.sourceGridX = cellIndex % 3;
                src.sourceGridY = cellIndex / 3;
                return src;
            }
        }
    }

    return src;
}

CarboxSource GetVanillaCarboxSource(const std::string& internalName) {
    CarboxSource src;

    // Search through the 5 vanilla carboxes (9 slots each)
    for (int gridIndex = 0; gridIndex < 5; ++gridIndex) {
        for (int i = 0; i < 9; ++i) {
            // Check if the slot isn't empty and matches our target name
            if (carboxGridNames[gridIndex][i] != nullptr && 
                internalName == carboxGridNames[gridIndex][i]) {
                
                src.filepath = "cars/misc/carbox" + std::to_string(gridIndex + 1) + ".bmp";
                src.isFromGrid = true;
                src.sourceGridX = i % 3;
                src.sourceGridY = i / 3;
                
                return src;
            }
        }
    }

    // If not found in the vanilla grid, it defaults to a standalone car texture
    src.filepath = "cars/" + internalName + "/carbox.bmp";
    src.isFromGrid = false;
    src.sourceGridX = 0;
    src.sourceGridY = 0;

    return src;
}

std::vector<CarboxSource> GetGridSourcesForCarbox(int carboxNumber, CarInfo* carPool) {
    std::vector<CarboxSource> sources;
    sources.reserve(9);

    // Convert carboxNumber (1 to 5) to a 0-based grid index
    int gridIndex = carboxNumber - 1;
    bool hasChanges = false;

    for (int i = 0; i < 9; ++i) {
        int carIdx = carboxGridIndexes[gridIndex][i];
        std::string defaultCarName = carboxGridNames[gridIndex][i];

        if (carIdx != -1) {
            CarInfo* car = &carPool[carIdx];
            std::string carName = car->internalName;

            // Use strcmp to compare the actual string contents (0 means they are identical)
            if (carName != defaultCarName) {
                hasChanges = true;
            }

            CarboxSource carboxSrc = GetCarboxSource(car);
            sources.push_back(carboxSrc);
        } else {
            // -1 matches the empty slots in carbox5
            // push an empty/transparent source placeholder.
            CarboxSource emptySrc;
            emptySrc.filepath = "";
            emptySrc.isFromGrid = false;
            emptySrc.sourceGridX = 0;
            emptySrc.sourceGridY = 0;
            sources.push_back(emptySrc);
        }
    }

    // If no cars have changed in this carbox file, send an empty list to signal we don't need to build a new .bmp
    if (!hasChanges) {
      sources.clear();
    }
    
    return sources;
}

// Returns the carbox number (1-5) if matched case-insensitively, or 0 if it doesn't match.
int GetCarboxNumberFromPath(const char* path) {
    if (path == nullptr) return 0;
    
    size_t len = strlen(path);
    if (len < 11) return 0; // "carbox1.bmp" is exactly 11 characters

    // Get a pointer to exactly 11 characters from the end of the string
    const char* tail = path + (len - 11);

    // Check if the first 6 characters of the tail are "carbox" (case-insensitive)
    if (_strnicmp(tail, "carbox", 6) == 0) {
        
        // Grab the 7th character (the number)
        char numChar = tail[6];
        
        // Ensure the number is strictly between 1 and 5
        if (numChar >= '1' && numChar <= '5') {
            
            // Check if the remaining part is exactly ".bmp" (case-insensitive)
            if (_stricmp(tail + 7, ".bmp") == 0) {
                
                // Convert the ASCII character to its integer value (e.g., '1' becomes 1)
                return numChar - '0';
            }
        }
    }
    
    return 0;
}

}