#include "TextureManager.hpp"

#include <string>

TextureManager* TextureManager::instance_ = nullptr;

TextureManager& TextureManager::Get() { return *instance_; }

void TextureManager::Init() {
  EASSERT_MSG(instance_ == nullptr, "Can't make two instances");
  instance_ = new TextureManager;
}

void TextureManager::Shutdown() {
  ZoneScoped;
  EASSERT_MSG(instance_ != nullptr, "Can't shutdown before initializing");
  delete instance_;
}

TextureManager::TextureManager() {
  EASSERT_MSG(instance_ == nullptr, "Cannot create two instances.");
  instance_ = this;
}

TextureHandle TextureManager::Create2dArray(
    const std::string& param_path, std::unordered_map<std::string, uint32_t>& name_to_idx) {
  static std::hash<std::string> hasher;
  uint32_t handle = hasher(param_path);
  texture_2d_array_map_.try_emplace(handle,
                                    std::make_shared<Texture2dArray>(param_path, name_to_idx));
  return handle;
}

TextureHandle TextureManager::Create2dArray(const Texture2dArrayCreateParams& params) {
  uint32_t handle = next_tex_array_handle_++;
  texture_2d_array_map_.emplace(handle, std::make_shared<Texture2dArray>(params));
  return handle;
}

const Texture2dArray& TextureManager::GetTexture2dArray(TextureHandle handle) {
  EASSERT_MSG(texture_2d_array_map_.contains(handle), "Texture not found");
  return *texture_2d_array_map_.at(handle).get();
}

void TextureManager::Remove2dArray(uint32_t handle) { texture_2d_array_map_.erase(handle); }

std::shared_ptr<Texture2D> TextureManager::Load(const std::string& name,
                                                const Texture2DCreateParamsEmpty& params) {
  EASSERT_MSG(name != "", "Name cannot be empty");
  auto it = texture_2d_map_.find(name);
  if (it != texture_2d_map_.end()) {
    return it->second;
  }
  auto tex = std::make_shared<Texture2D>(params);
  texture_2d_map_.emplace(name, tex);
  return tex;
}

std::shared_ptr<Texture2D> TextureManager::Load(const Texture2DCreateParams& params) {
  auto it = texture_2d_map_.find(params.filepath);
  if (it != texture_2d_map_.end()) {
    return it->second;
  }
  auto tex = std::make_shared<Texture2D>(params);
  texture_2d_map_.emplace(params.filepath, tex);
  return tex;
}

void TextureManager::RemoveUnused() {
  std::vector<std::string> to_delete;
  for (auto& [name, mat] : texture_2d_map_) {
    if (mat.use_count() == 1) to_delete.emplace_back(name);
  }
  for (const auto& name : to_delete) {
    texture_2d_map_.erase(name);
  }
}
