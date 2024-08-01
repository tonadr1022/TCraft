#include "Texture2d.hpp"

#include <glm/common.hpp>
#include <glm/exponential.hpp>

#include "pch.hpp"
#include "util/LoadFile.hpp"

Texture2D::Texture2D(const Texture2DCreateParams& params) {
  glCreateTextures(GL_TEXTURE_2D, 1, &id_);

  Image image;
  if (!params.filepath.empty()) {
    util::LoadImage(image, params.filepath, params.flip);
  }
  dims_.x = image.width;
  dims_.y = image.height;

  GLuint mip_levels = 0;
  if (params.generate_mipmaps) {
    mip_levels =
        static_cast<GLuint>(glm::ceil(glm::log2(static_cast<float>(glm::min(dims_.x, dims_.y)))));
  }

  // TODO(tony): separate into sampler and/or set in params
  glTextureParameteri(id_, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTextureParameteri(id_, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTextureParameteri(id_, GL_TEXTURE_MIN_FILTER, params.filter_mode_min);
  glTextureParameteri(id_, GL_TEXTURE_MAG_FILTER, params.filter_mode_max);

  glTextureStorage2D(id_, mip_levels, params.internal_format, dims_.x, dims_.y);

  glTextureSubImage2D(id_,
                      0,                 // first mip level
                      0,                 // x offset
                      0,                 // y offset
                      dims_.x,           // width
                      dims_.y,           // height
                      GL_RGBA,           // format
                      GL_UNSIGNED_BYTE,  // type
                      image.pixels       // data
  );

  if (params.generate_mipmaps) glGenerateTextureMipmap(id_);

  // https://www.khronos.org/opengl/wiki/Bindless_Texture
  if (params.bindless) {
    bindless_handle_ = glGetTextureHandleARB(id_);
    glMakeTextureHandleResidentARB(bindless_handle_);
  }
}

Texture2D::~Texture2D() {
  if (bindless_handle_) {
    MakeNonResident();
  }
  if (id_) {
    glDeleteTextures(1, &id_);
  }
}

Texture2D::Texture2D(Texture2D&& other) noexcept { *this = std::move(other); }

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept {
  this->id_ = std::exchange(other.id_, 0);
  this->dims_ = other.dims_;
  this->bindless_handle_ = std::exchange(other.bindless_handle_, 0);
  return *this;
}

void Texture2D::MakeNonResident() const { glMakeTextureHandleNonResidentARB(bindless_handle_); }

bool Texture2D::IsValid() const { return id_ != 0; }
