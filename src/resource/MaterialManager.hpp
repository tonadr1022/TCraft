#pragma once

#include "renderer/Material.hpp"

struct Texture2DCreateParams;
struct Texture2DCreateParamsEmpty;
struct Texture2DArrayCreateParams;

class MaterialManager {
 public:
  static MaterialManager& Get();
  static void Init();
  static void Shutdown();
  std::shared_ptr<TextureMaterial> LoadTextureMaterial(const Texture2DCreateParams& params);
  std::shared_ptr<TextureMaterial> LoadTextureMaterial(const std::string& name,
                                                       const Texture2DCreateParamsEmpty& params);
  std::shared_ptr<TextureMaterial> LoadTextureMaterial(const Texture2DArrayCreateParams& params);
  void Erase(const std::string& name);
  void RemoveUnused();

 private:
  uint32_t next_handle_{1};
  static MaterialManager* instance_;

  std::unordered_map<std::string, std::shared_ptr<TextureMaterial>> texture_mat_map_;
};
