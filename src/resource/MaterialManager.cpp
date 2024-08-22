#include "MaterialManager.hpp"

#include "renderer/Material.hpp"
#include "renderer/opengl/Texture2d.hpp"
#include "resource/TextureManager.hpp"

MaterialManager* MaterialManager::instance_ = nullptr;

MaterialManager& MaterialManager::Get() { return *instance_; }

void MaterialManager::Init() {
  EASSERT_MSG(instance_ == nullptr, "Can't make two instances");
  instance_ = new MaterialManager;
}

void MaterialManager::Shutdown() {
  ZoneScoped;
  EASSERT_MSG(instance_ != nullptr, "Can't shutdown before initializing");
  delete instance_;
}

void MaterialManager::Erase(const std::string& name) { texture_mat_map_.erase(name); }

std::shared_ptr<TextureMaterial> MaterialManager::LoadTextureMaterial(
    const std::string& name, const Texture2DCreateParamsEmpty& params) {
  auto it = texture_mat_map_.find(name);
  if (it != texture_mat_map_.end()) {
    return it->second;
  }

  auto tex = TextureManager::Get().Load(name, params);
  TextureMaterialData data{tex->BindlessHandle()};
  auto mat = std::make_shared<TextureMaterial>(data, tex);
  texture_mat_map_.emplace(name, mat);
  return mat;
}

std::shared_ptr<TextureMaterial> MaterialManager::LoadTextureMaterial(
    const Texture2DCreateParams& params) {
  auto tex = TextureManager::Get().Load(params);
  auto it = texture_mat_map_.find(params.filepath);
  if (it != texture_mat_map_.end()) {
    return it->second;
  }

  TextureMaterialData data{tex->BindlessHandle()};
  auto mat = std::make_shared<TextureMaterial>(data, tex);
  texture_mat_map_.emplace(params.filepath, mat);
  return mat;
}

void MaterialManager::RemoveUnused() {
  std::vector<std::string> to_delete;
  for (auto& [name, mat] : texture_mat_map_) {
    if (mat.use_count() == 1) {
      to_delete.emplace_back(name);
    }
  }

  for (const auto& name : to_delete) {
    texture_mat_map_.erase(name);
  }
}
