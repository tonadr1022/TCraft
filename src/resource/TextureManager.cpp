#include "TextureManager.hpp"

#include <string>

#include "renderer/opengl/Texture2dArray.hpp"

TextureManager& TextureManager::Get() {
  static TextureManager instance;
  return instance;
}

TextureHandle TextureManager::Create2dArray(
    const std::string& param_path, std::unordered_map<std::string, uint32_t>& name_to_idx) {
  static std::hash<std::string> hasher;
  uint32_t handle = hasher(param_path);
  texture_2d_array_map_.emplace(handle, Texture2dArray(param_path, name_to_idx));
  return handle;
}

TextureManager::TextureManager() = default;
