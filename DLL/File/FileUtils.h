#pragma once

#include <string>

namespace Randomizer {

void InitializePacklistCache();
std::string GetAbsoluteFilePath(const std::string& relativePath);

}