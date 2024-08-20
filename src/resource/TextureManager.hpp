#pragma once

#include "renderer/opengl/Texture2d.hpp"
#include "renderer/opengl/Texture2dArray.hpp"

using TextureHandle = uint32_t;

class TextureManager {
 public:
  static void Init();
  static void Shutdown();
  static TextureManager& Get();
  const Texture2dArray& GetTexture2dArray(TextureHandle handle);
  TextureHandle Create2dArray(const Texture2dArrayCreateParams& params);
  TextureHandle Create2dArray(const std::string& param_path,
                              std::unordered_map<std::string, uint32_t>& name_to_idx);
  void Remove2dArray(uint32_t handle);

  [[nodiscard]] std::shared_ptr<Texture> Load(const std::string& name,
                                              const TextureCubeCreateParamsPaths& params);
  // [[nodiscard]] std::shared_ptr<Texture> Load(const std::string& name,
  //                                             const TextureCubeCreateParams& params);
  [[nodiscard]] std::shared_ptr<Texture> Load(const Texture2DCreateParams& params);
  [[nodiscard]] std::shared_ptr<Texture> Load(const std::string& name,
                                              const Texture2DCreateParamsEmpty& params);
  void RemoveTexture2D(const std::string& key);
  void RemoveUnusedTextures();

 private:
  static TextureManager* instance_;
  TextureManager();
  uint32_t next_tex_array_handle_{1};

  std::unordered_map<uint32_t, std::shared_ptr<Texture2dArray>> texture_2d_array_map_;
  std::unordered_map<std::string, std::shared_ptr<Texture>> texture_map_;
};
