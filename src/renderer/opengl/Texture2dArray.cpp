#include "Texture2dArray.hpp"

#include <glm/common.hpp>
#include <glm/exponential.hpp>

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

Texture2dArray::Texture2dArray(const Texture2dArrayCreateParams& params) {
  glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &id_);

  glTextureParameteri(id_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTextureParameteri(id_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTextureParameteri(id_, GL_TEXTURE_MIN_FILTER, params.filter_mode_min);
  glTextureParameteri(id_, GL_TEXTURE_MAG_FILTER, params.filter_mode_max);

  GLuint mip_levels = 1;
  if (params.generate_mipmaps) {
    mip_levels =
        static_cast<GLuint>(glm::ceil(glm::log2(static_cast<float>(glm::min(dims_.x, dims_.y)))));
  }

  glTextureStorage3D(id_, mip_levels, params.internal_format, dims_.x, dims_.y,
                     params.all_pixels_data.size());

  int i = 0;
  for (int i = 0; i < params.all_pixels_data.size(); i++) {
    glTextureSubImage3D(id_, 0, 0, 0, i, params.dims.x, params.dims.y, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                        params.all_pixels_data[i]);
  }

  if (params.generate_mipmaps) glGenerateTextureMipmap(id_);
  spdlog::info("generated texture array");
}

bool Texture2dArray::IsValid() const { return id_ != 0; }
