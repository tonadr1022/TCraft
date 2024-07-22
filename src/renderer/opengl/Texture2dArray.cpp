#include "Texture2dArray.hpp"

#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <nlohmann/json.hpp>

#include "util/JsonUtil.hpp"
#include "util/LoadFile.hpp"
#include "util/StringUtil.hpp"

Texture2dArray::Texture2dArray(Texture2dArray&& other) noexcept
    : id_(std::exchange(other.id_, 0)), dims_(other.dims_) {}

Texture2dArray& Texture2dArray::operator=(Texture2dArray&& other) noexcept {
  this->id_ = std::exchange(other.id_, 0);
  this->dims_ = other.dims_;
  return *this;
}

Texture2dArray::~Texture2dArray() {
  spdlog::info("delete tex array");
  if (id_) {
    glDeleteTextures(1, &id_);
  }
}

void Texture2dArray::LoadFromParams(const Texture2dArrayCreateParams& params) {
  ZoneScoped;

  dims_ = params.dims;
  glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &id_);
  glTextureParameteri(id_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTextureParameteri(id_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTextureParameteri(id_, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTextureParameteri(id_, GL_TEXTURE_MIN_FILTER, params.filter_mode_min);
  glTextureParameteri(id_, GL_TEXTURE_MAG_FILTER, params.filter_mode_max);

  uint32_t mip_levels = 1;
  if (params.generate_mipmaps) {
    mip_levels = glm::floor(glm::log2(static_cast<float>(glm::max(dims_.x, dims_.y)))) + 1;
  }

  glTextureStorage3D(id_, mip_levels, params.internal_format, dims_.x, dims_.y,
                     params.all_pixels_data.size());

  for (int i = 0; i < params.all_pixels_data.size(); i++) {
    glTextureSubImage3D(id_, 0, 0, 0, i, params.dims.x, params.dims.y, 1, params.format,
                        GL_UNSIGNED_BYTE, params.all_pixels_data[i]);
  }
  if (params.generate_mipmaps) glGenerateTextureMipmap(id_);
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
  static const std::unordered_map<std::string, uint32_t> FormatMap = {
      {"rgba8", GL_RGBA8},
      {"rgba", GL_RGBA},
      {"rgb8", GL_RGB8},
      {"rgb", GL_RGB},
  };

  auto filter_max = json_util::GetString(data, "filter_max");
  auto filter_min = json_util::GetString(data, "filter_min");
  uint32_t filter_mode_min = FilterModeMap.at(filter_min.value());
  uint32_t filter_mode_max = FilterModeMap.at(filter_max.value());
  auto internal_format_str = json_util::GetString(data, "internal_format");
  auto format_str = json_util::GetString(data, "format");
  uint32_t internal_format = FormatMap.at(internal_format_str.value());
  uint32_t format = FormatMap.at(format_str.value());

  auto path_base = std::string(TEXTURE_PATH);
  int tex_idx = 0;
  for (auto& texture : data["textures"]) {
    auto texture_str = texture.get<std::string>();
    name_to_idx[util::GetFilenameStem(texture_str)] = tex_idx;
    Image image;
    util::LoadImage(image, path_base + texture_str, true);
    if (image.width != width || image.height != height) {
      spdlog::error("invalid texture {}", texture_str);
    } else {
      all_pixels_data.emplace_back(image.pixels);
      tex_idx++;
    }
  }
  LoadFromParams({
      .all_pixels_data = all_pixels_data,
      .dims = glm::ivec2{width, height},
      .generate_mipmaps = true,
      .internal_format = internal_format,
      .format = format,
      .filter_mode_min = filter_mode_min,
      .filter_mode_max = filter_mode_max,
  });
}

bool Texture2dArray::IsValid() const { return id_ != 0; }

void Texture2dArray::Bind(int unit) const { glBindTextureUnit(unit, id_); }
