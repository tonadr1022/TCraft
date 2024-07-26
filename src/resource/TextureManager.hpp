#pragma once

#include "renderer/opengl/Texture2dArray.hpp"

using TextureHandle = uint32_t;
struct Texture2dArrayCreateParams;

class TextureManager {
 public:
  static TextureManager& Get();
  const Texture2dArray& GetTexture2dArray(TextureHandle handle);
  TextureHandle Create2dArray(const Texture2dArrayCreateParams& params);
  TextureHandle Create2dArray(const std::string& param_path,
                              std::unordered_map<std::string, uint32_t>& name_to_idx);
  void Remove2dArray(uint32_t handle);

 private:
  friend class Application;
  static TextureManager* instance_;
  TextureManager();
  uint32_t next_tex_array_handle_{1};

  std::unordered_map<uint32_t, Texture2dArray> texture_2d_array_map_;
};
