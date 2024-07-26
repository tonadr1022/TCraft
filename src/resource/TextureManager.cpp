#include "TextureManager.hpp"

#include <string>

#include "renderer/opengl/Texture2dArray.hpp"

TextureManager* TextureManager::instance_ = nullptr;
TextureManager& TextureManager::Get() { return *instance_; }
TextureManager::TextureManager() {
  EASSERT_MSG(instance_ == nullptr, "Cannot create two instances.");
  instance_ = this;
}

TextureHandle TextureManager::Create2dArray(
    const std::string& param_path, std::unordered_map<std::string, uint32_t>& name_to_idx) {
  static std::hash<std::string> hasher;
  uint32_t handle = hasher(param_path);
  texture_2d_array_map_.try_emplace(handle, param_path, name_to_idx);
  return handle;
}

TextureHandle TextureManager::Create2dArray(const Texture2dArrayCreateParams& params) {
  uint32_t handle = next_tex_array_handle_++;
  texture_2d_array_map_.try_emplace(handle, params);
  return handle;
}

const Texture2dArray& TextureManager::GetTexture2dArray(TextureHandle handle) {
  EASSERT_MSG(texture_2d_array_map_.contains(handle), "Texture not found");
  return texture_2d_array_map_.at(handle);
}

void TextureManager::Remove2dArray(uint32_t handle) { texture_2d_array_map_.erase(handle); }
