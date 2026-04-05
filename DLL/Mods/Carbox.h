#pragma once
#include <string>
#include <vector>
#include "RVGLStructs.h"

namespace Randomizer {

struct CarboxSource {
    std::string filepath;
    bool isFromGrid;     // true if it's carbox1.bmp to carbox5.bmp
    int sourceGridX;     // 0, 1, or 2 (used only if isFromGrid is true)
    int sourceGridY;     // 0, 1, or 2 (used only if isFromGrid is true)
};

// Public functions accessible by other files
CarboxSource GetCarboxSource(int carIdx, CarInfo* carPool);
CarboxSource GetVanillaCarboxSource(const std::string& internalName);
std::vector<CarboxSource> GetGridSourcesForCarbox(int carboxNumber, CarInfo* carPool);
int GetCarboxNumberFromPath(const char* path);
std::string GetCarboxAbsoluteFolderPath();

}