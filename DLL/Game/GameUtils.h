#pragma once
#include <string>

struct CarInfo;

namespace Randomizer {

  bool IsStockCar(const std::string internalName);
  // Accepts a bare folder or a cars/ or cars\ path, preserving folder casing.
  std::string GetCarFolderName(const std::string& path);
  // Reads the fixed-size native name without relying on a terminating null byte.
  std::string GetCarFolderName(const CarInfo& car);
  bool IsSameCarFolder(const std::string& left, const char* right);

}