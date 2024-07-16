#pragma once

#include <optional>
#include <string>

#include "resource/Image.hpp"

namespace util {
extern std::optional<std::string> LoadFromFile(const std::string& path);
extern void LoadImage(Image& image, std::string_view path, bool flip);
}  // namespace util
