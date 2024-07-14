#include "Texture2dArray.hpp"

Texture2dArray::Texture2dArray(Texture2dArray&& other) noexcept
    : id_(other.id_), dims_(other.dims_) {}

Texture2dArray& Texture2dArray::operator=(Texture2dArray&& other) noexcept {
  this->id_ = std::exchange(other.id_, 0);
  this->dims_ = other.dims_;
  return *this;
}

Texture2dArray::Texture2dArray(const std::vector<std::vector<uint8_t>>& pixels,
                               const glm::ivec2& dims)
    : dims_(dims) {
  glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &id_);
  // TODO: AZDO texture 2d array load
}

bool Texture2dArray::IsValid() const { return id_ != 0; }
