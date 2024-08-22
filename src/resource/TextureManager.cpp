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

std::shared_ptr<Texture> TextureManager::Load(const Texture2DArrayCreateParams& params) {
  uint32_t handle = next_tex_array_handle_++;
  auto tex = std::make_shared<Texture>(params);
  texture_map_.emplace(std::to_string(handle), tex);
  return tex;
}

std::shared_ptr<Texture> TextureManager::Load(const std::string& name,
                                              const TextureCubeCreateParamsPaths& params) {
  auto it = texture_map_.find(name);
  if (it != texture_map_.end()) {
    return it->second;
  }
  auto tex = std::make_shared<Texture>(params);
  texture_map_.emplace(name, tex);
  return tex;
}

// TODO: templatize
std::shared_ptr<Texture> TextureManager::Load(const std::string& name,
                                              const Texture2DCreateParamsEmpty& params) {
  auto it = texture_map_.find(name);
  if (it != texture_map_.end()) {
    return it->second;
  }
  auto tex = std::make_shared<Texture>(params);
  texture_map_.emplace(name, tex);
  return tex;
}

std::shared_ptr<Texture> TextureManager::Load(const Texture2DCreateParams& params) {
  auto it = texture_map_.find(params.filepath);
  if (it != texture_map_.end()) {
    return it->second;
  }
  auto tex = std::make_shared<Texture>(params);
  texture_map_.emplace(params.filepath, tex);
  return tex;
}

void TextureManager::RemoveUnusedTextures() {
  std::vector<std::string> to_delete;
  for (auto& [name, mat] : texture_map_) {
    if (mat.use_count() == 1) to_delete.emplace_back(name);
  }
  for (const auto& name : to_delete) {
    texture_map_.erase(name);
  }
}
