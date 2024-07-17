#include "LoadFile.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image/stb_image_write.h>

namespace util {

std::optional<std::string> LoadFromFile(const std::string& path) {
  std::ifstream file_stream(path);
  std::string line;
  std::stringstream s_stream;
  if (!file_stream.is_open()) return std::nullopt;
  while (std::getline(file_stream, line)) {
    s_stream << line << '\n';
  }
  return s_stream.str();
}

void LoadImage(Image& image, std::string_view path, bool flip) {
  stbi_set_flip_vertically_on_load(flip);
  int comp;
  if (!path.empty()) {
    image.pixels = stbi_load(path.data(), &image.width, &image.height, &image.channels, 4);
    if (!image.pixels) {
      spdlog::error("Failed to load texture at path {}", path);
    }
  }
}

nlohmann::json LoadJsonFile(const std::string& path) {
  std::ifstream file_stream(path);
  if (!file_stream.is_open()) {
    throw std::runtime_error("Failed to open json file: %s" + path);
  }
  return nlohmann::json::parse(file_stream);
}

}  // namespace util
