#include "Texture2dArray.hpp"

#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <nlohmann/json.hpp>

#include "util/LoadFile.hpp"
#include "util/Paths.hpp"
#include "util/StringUtil.hpp"

Texture2dArray::Texture2dArray(Texture2dArray&& other) noexcept
    : id_(other.id_), dims_(other.dims_) {}

Texture2dArray& Texture2dArray::operator=(Texture2dArray&& other) noexcept {
  this->id_ = std::exchange(other.id_, 0);
  this->dims_ = other.dims_;
  return *this;
}
Texture2dArray::~Texture2dArray() {
  if (id_) {
    glDeleteTextures(1, &id_);
  }
}

void Texture2dArray::LoadFromParams(const Texture2dArrayCreateParams& params) {
  ZoneScoped;
  dims_ = params.dims;
  glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &id_);
  spdlog::info("id {}", id_);
  glTextureParameteri(id_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTextureParameteri(id_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTextureParameteri(id_, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTextureParameteri(id_, GL_TEXTURE_MIN_FILTER, params.filter_mode_min);
  glTextureParameteri(id_, GL_TEXTURE_MAG_FILTER, params.filter_mode_max);

  GLuint mip_levels = 1;
  if (params.generate_mipmaps) {
    mip_levels =
        static_cast<GLuint>(glm::ceil(glm::log2(static_cast<float>(glm::min(dims_.x, dims_.y)))));
  }

  glTextureStorage3D(id_, mip_levels, params.internal_format, dims_.x, dims_.y,
                     params.all_pixels_data.size());

  for (int i = 0; i < params.all_pixels_data.size(); i++) {
    glTextureSubImage3D(id_, 0, 0, 0, i, params.dims.x, params.dims.y, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                        params.all_pixels_data[i]);
  }
  if (params.generate_mipmaps) glGenerateTextureMipmap(id_);
}

void Texture2dArray::LoadFromParams2(const Texture2dArrayCreateParams& params) {
  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &id_);
  glBindTexture(GL_TEXTURE_2D_ARRAY, id_);

  glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, params.dims.x, params.dims.y,
               params.all_pixels_data.size(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

  // configure sampler
  // s = x-axis
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
  // t = y-axis
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // create sub images on GPU
  for (int index = 0; index < params.all_pixels_data.size(); index++) {
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, index, static_cast<GLsizei>(params.dims.x),
                    static_cast<GLsizei>(params.dims.y), 1, GL_RGBA, GL_UNSIGNED_BYTE,
                    params.all_pixels_data[index]);
  }

  glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}
Texture2dArray::Texture2dArray(const Texture2dArrayCreateParams& params) { LoadFromParams(params); }

Texture2dArray::Texture2dArray(const std::string& param_path,
                               std::unordered_map<std::string, uint32_t>& name_to_idx) {
  ZoneScoped;
  nlohmann::json data = util::LoadJsonFile(param_path);
  std::vector<void*> all_pixels_data;
  int width = data["width"].get<int>();
  int height = data["height"].get<int>();

  static const std::unordered_map<std::string, uint32_t> FilterModeMap = {
      {"nearest", GL_NEAREST},
      {"linear", GL_LINEAR},
      {"nearest_mipmap_linear", GL_NEAREST_MIPMAP_LINEAR}};

  auto filter_max = data["filter_max"].get<std::string>();
  if (!FilterModeMap.contains(filter_max)) {
    spdlog::error("Invalid filter mode {} in 2d array {}", filter_max, param_path);
    return;
  }
  auto filter_min = data["filter_min"].get<std::string>();
  if (!FilterModeMap.contains(filter_min)) {
    spdlog::error("Invalid filter mode {} in 2d array {}", filter_min, param_path);
    return;
  }

  uint32_t filter_mode_min = FilterModeMap.at(filter_min);
  uint32_t filter_mode_max = FilterModeMap.at(filter_max);

  auto path_base = std::string(TEXTURE_PATH);
  int tex_idx = 0;
  for (auto& texture : data["textures"]) {
    auto texture_str = texture.get<std::string>();
    name_to_idx[util::GetFilenameStem(texture_str)] = tex_idx;
    Image image;
    util::LoadImage(image, path_base + texture_str, true);
    if (image.width != width || image.height != height) {
    } else {
      all_pixels_data.emplace_back(image.pixels);
      tex_idx++;
    }
  }
  LoadFromParams({
      .all_pixels_data = all_pixels_data,
      .dims = glm::ivec2{width, height},
      .generate_mipmaps = true,
      .internal_format = GL_RGBA8,
      .filter_mode_min = filter_mode_min,
      .filter_mode_max = filter_mode_max,
  });
}

bool Texture2dArray::IsValid() const { return id_ != 0; }

void Texture2dArray::Bind(int unit) const { glBindTextureUnit(unit, id_); }
