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

Texture2dArray& TextureManager::GetTexture2dArray(TextureHandle handle) {
  // TODO: handle gracefully
  if (!texture_2d_array_map_.contains(handle)) {
    exit(1);
  }
  return texture_2d_array_map_.at(handle);
}
TextureManager::TextureManager() = default;
