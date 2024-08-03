#pragma once

#include "renderer/opengl/Texture2d.hpp"
#include "renderer/opengl/Texture2dArray.hpp"

using TextureHandle = uint32_t;

class TextureManager {
 public:
  static TextureManager& Get();
  const Texture2dArray& GetTexture2dArray(TextureHandle handle);
  TextureHandle Create2dArray(const Texture2dArrayCreateParams& params);
  TextureHandle Create2dArray(const std::string& param_path,
                              std::unordered_map<std::string, uint32_t>& name_to_idx);
  void Remove2dArray(uint32_t handle);

  [[nodiscard]] std::shared_ptr<Texture2D> Load(const Texture2DCreateParams& params);
  void RemoveTexture2D(TextureHandle handle);
  void RemoveUnused();

 private:
  friend class Application;
  static TextureManager* instance_;
  TextureManager();
  uint32_t next_tex_array_handle_{1};

  std::unordered_map<uint32_t, std::shared_ptr<Texture2dArray>> texture_2d_array_map_;
  std::unordered_map<std::string, std::shared_ptr<Texture2D>> texture_2d_map_;
};
