#include "LoadFile.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image/stb_image_write.h>

#pragma clang diagnostic pop

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

void LoadImage(Image& image, std::string_view path, int required_channels, bool flip) {
  stbi_set_flip_vertically_on_load(flip);
  if (!path.empty()) {
    image.pixels =
        stbi_load(path.data(), &image.width, &image.height, &image.channels, required_channels);
    if (!image.pixels) {
      spdlog::error("Failed to load image at path {}", path);
    }
    // stbi_image_free(image.pixels);
  }
}
void FreeImage(void* pixels) { stbi_image_free(pixels); }

nlohmann::json LoadJsonFile(const std::string& path) {
  std::ifstream file_stream(path);
  if (!file_stream.is_open()) {
    throw std::runtime_error("Failed to open json file: %s" + path);
  }
  try {
    return nlohmann::json::parse(file_stream);
  } catch (const nlohmann::json::parse_error& e) {
    return {};
  }
}

void WriteImage(uint32_t tex, uint32_t num_channels, const std::string& out_path) {
  stbi_flip_vertically_on_write(true);
  int w, h;
  int mip_level = 0;
  glGetTextureLevelParameteriv(tex, mip_level, GL_TEXTURE_WIDTH, &w);
  glGetTextureLevelParameteriv(tex, mip_level, GL_TEXTURE_HEIGHT, &h);
  std::vector<uint8_t> pixels(w * h * num_channels);
  glGetTextureImage(tex, mip_level, num_channels == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE,
                    sizeof(uint8_t) * pixels.size(), pixels.data());
  stbi_write_png(out_path.c_str(), w, h, num_channels, pixels.data(), w * num_channels);
}

}  // namespace util
