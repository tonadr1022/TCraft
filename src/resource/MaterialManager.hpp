#pragma once

#include "renderer/Material.hpp"

struct Texture2DCreateParams;

class MaterialManager {
 public:
  static MaterialManager& Get();
  static void Init();
  static void Shutdown();
  std::shared_ptr<TextureMaterial> LoadTextureMaterial(const Texture2DCreateParams& params);
  void RemoveUnused();

 private:
  static MaterialManager* instance_;

  std::unordered_map<std::string, std::shared_ptr<TextureMaterial>> texture_mat_map_;
};
