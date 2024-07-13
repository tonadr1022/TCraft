#pragma once

#include <optional>
#include <string>

namespace util {
extern std::optional<std::string> LoadFromFile(const std::string& path);
}
