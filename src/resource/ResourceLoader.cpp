#include "ResourceLoader.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

#include "renderer/opengl/Texture2dArray.hpp"
#include "util/LoadFile.hpp"
#include "util/Paths.hpp"

void ResourceLoader::LoadResources() {
  std::ifstream f(GET_PATH("resources/data/block/textures.json"));
  nlohmann::json textures = nlohmann::json::parse(f);
  std::vector<void*> all_pixels_data;
  int texture_res = 16;
  int i = 0;
  for (auto& texture : textures) {
    auto texture_str = texture.get<std::string>();
    block_texture_filename_to_tex_index_[texture_str] = i++;
    auto path = GET_PATH("resources/textures/block/") + texture_str;
    Image image;
    util::LoadImage(image, path, true);
    if (image.width != texture_res || image.height != texture_res) {
      // spdlog::error("{} {} texture {} is  ", image.width, image.height, path);
    } else {
      all_pixels_data.emplace_back(image.pixels);
    }
  }
  texture_2d_array_map_.emplace("blocks", Texture2dArray{{
                                              .all_pixels_data = all_pixels_data,
                                              .dims = glm::ivec2{texture_res, texture_res},
                                              .generate_mipmaps = true,
                                              .internal_format = GL_RGBA8,
                                              .filter_mode_min = GL_REPEAT,
                                              .filter_mode_max = GL_REPEAT,
                                          }});
}
