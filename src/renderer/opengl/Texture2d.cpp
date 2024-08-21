#include "Texture2d.hpp"

#include <glm/common.hpp>
#include <glm/exponential.hpp>

#include "pch.hpp"
#include "resource/Image.hpp"
#include "util/LoadFile.hpp"

namespace {
uint32_t GetMipLevels(int width, int height) {
  return static_cast<GLuint>(glm::ceil(glm::log2(static_cast<float>(glm::min(width, height)))));
}
}  // namespace

// Texture::Texture(const TextureCubeCreateParams& params) {
//   glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id_);
//   Image image;
//   util::LoadImage(image, params.path, 0);
//
//   spdlog::info("{} {} {}", image.width / 6, image.height, image.channels);
//   GLenum format = image.channels == 3 ? GL_RGB : GL_RGBA;
//   GLenum internal_format = image.channels == 3 ? GL_RGB8 : GL_RGBA8;
//   glTextureStorage2D(id_, 1, internal_format, image.width / 6, image.height);
//
//   int face_width = image.width / 6;
//
//   for (int face = 0; face < 6; face++) {
//     unsigned char* face_data =
//         static_cast<unsigned char*>(image.pixels) + (face * face_width * image.channels);
//
//     // Copy the correct portion for each face
//     glTextureSubImage3D(id_, 0, 0, 0, face, face_width, image.height, 1, format,
//     GL_UNSIGNED_BYTE,
//                         face_data);
//   }
//   util::FreeImage(image.pixels);
//   glTextureParameteri(id_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//   glTextureParameteri(id_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//   glTextureParameteri(id_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//   glTextureParameteri(id_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//   glTextureParameteri(id_, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
// }

Texture::Texture(const TextureCubeCreateParamsPaths& params) {
  glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id_);
  std::vector<Image> images(6);
  for (int i = 0; i < 6; i++) {
    Image image;
    util::LoadImage(image, params.paths[i], 0, false);
    images[i] = image;
  }
  GLenum internal_format = images[0].channels == 3 ? GL_RGB8 : GL_RGBA8;
  GLenum format = images[0].channels == 3 ? GL_RGB : GL_RGBA;
  glTextureStorage2D(id_, 1, internal_format, images[0].width, images[0].height);
  int i = 0;
  for (auto& image : images) {
    glTextureSubImage3D(id_, 0, 0, 0, i++, image.width, image.height, 1, format, GL_UNSIGNED_BYTE,
                        image.pixels);
    util::FreeImage(image.pixels);
  }
  glTextureParameteri(id_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTextureParameteri(id_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTextureParameteri(id_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTextureParameteri(id_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTextureParameteri(id_, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

Texture::Texture(const Texture2DCreateParamsEmpty& params) {
  glCreateTextures(GL_TEXTURE_2D, 1, &id_);
  dims_.x = params.width;
  dims_.y = params.height;
  glTextureParameteri(id_, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTextureParameteri(id_, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTextureParameteri(id_, GL_TEXTURE_MIN_FILTER, params.filter_mode_min);
  glTextureParameteri(id_, GL_TEXTURE_MAG_FILTER, params.filter_mode_max);

  uint32_t mip_levels = params.generate_mipmaps ? GetMipLevels(dims_.x, dims_.y) : 1;
  glTextureStorage2D(id_, mip_levels, GL_RGBA8, dims_.x, dims_.y);

  if (params.bindless) {
    bindless_handle_ = glGetTextureHandleARB(id_);
    MakeResident();
  }
  if (params.generate_mipmaps) glGenerateTextureMipmap(id_);
}

Texture::Texture(const Texture2DCreateParams& params) {
  glCreateTextures(GL_TEXTURE_2D, 1, &id_);

  Image image;
  if (!params.filepath.empty()) {
    util::LoadImage(image, params.filepath, 0, params.flip);
  }
  dims_.x = image.width;
  dims_.y = image.height;

  // TODO: separate into sampler and/or set in params
  glTextureParameteri(id_, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTextureParameteri(id_, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTextureParameteri(id_, GL_TEXTURE_MIN_FILTER, params.filter_mode_min);
  glTextureParameteri(id_, GL_TEXTURE_MAG_FILTER, params.filter_mode_max);

  uint32_t mip_levels = params.generate_mipmaps ? GetMipLevels(dims_.x, dims_.y) : 1;
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
  util::FreeImage(image.pixels);

  if (params.generate_mipmaps) glGenerateTextureMipmap(id_);

  // https://www.khronos.org/opengl/wiki/Bindless_Texture
  if (params.bindless) {
    bindless_handle_ = glGetTextureHandleARB(id_);
    MakeResident();
  }
}

Texture::~Texture() {
  ZoneScoped;
  if (resident_) {
    MakeNonResident();
  }
  if (id_) {
    glDeleteTextures(1, &id_);
  }
}

Texture::Texture(Texture&& other) noexcept { *this = std::move(other); }

Texture& Texture::operator=(Texture&& other) noexcept {
  this->id_ = std::exchange(other.id_, 0);
  this->dims_ = other.dims_;
  this->bindless_handle_ = std::exchange(other.bindless_handle_, 0);
  return *this;
}

void Texture::MakeNonResident() {
  EASSERT_MSG(resident_, "Must be resident in order to call make not resident");
  EASSERT_MSG(bindless_handle_ != 0, "Invalid binless handle");
  glMakeTextureHandleNonResidentARB(bindless_handle_);
  resident_ = false;
}

void Texture::MakeResident() {
  EASSERT_MSG(!resident_, "Cannot already be resident.");
  EASSERT_MSG(bindless_handle_ != 0, "Invalid binless handle");
  glMakeTextureHandleResidentARB(bindless_handle_);
  resident_ = true;
}

bool Texture::IsValid() const { return id_ != 0; }
