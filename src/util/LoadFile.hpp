#pragma once

#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>

#include "resource/Image.hpp"

namespace util {
extern std::optional<std::string> LoadFromFile(const std::string& path);
extern void LoadImage(Image& image, std::string_view path, int required_channels, bool flip = true);
extern void FreeImage(void* pixels);
void WriteImage(uint32_t tex, uint32_t num_channels, const std::string& out_path);
extern nlohmann::json LoadJsonFile(const std::string& path);
}  // namespace util
