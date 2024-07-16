#pragma once

#include <unordered_map>

#include "../renderer/opengl/Texture2dArray.hpp"

using TextureHandle = uint32_t;
class ResourceLoader {
 public:
  static void LoadResources();

  // TODO: separate from here
  inline static std::vector<std::string> block_textures_;

  inline static std::unordered_map<std::string, int> block_texture_filename_to_tex_index_;

 private:
  inline static std::unordered_map<std::string, Texture2dArray> texture_2d_array_map_;
};
