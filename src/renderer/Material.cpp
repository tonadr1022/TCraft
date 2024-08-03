#include "Material.hpp"

#include <utility>

#include "renderer/Renderer.hpp"

TextureMaterial::~TextureMaterial() {
  if (handle_) {
    spdlog::info("freeing texture material {}", handle_);
    Renderer::Get().FreeMaterial(handle_);
  }
  handle_ = 0;
}

TextureMaterial::TextureMaterial() = default;

TextureMaterial::TextureMaterial(TextureMaterialData& data, std::shared_ptr<Texture2D> tex)
    : tex_(std::move(tex)) {
  handle_ = Renderer::Get().AllocateMaterial(data);
};

TextureMaterial::TextureMaterial(TextureMaterial&& other) noexcept { *this = std::move(other); }

TextureMaterial& TextureMaterial::operator=(TextureMaterial&& other) noexcept {
  if (&other == this) return *this;
  this->~TextureMaterial();
  handle_ = std::exchange(other.handle_, 0);
  return *this;
}

uint32_t TextureMaterial::Handle() const {
  EASSERT_MSG(handle_, "Don't get a null handle");
  return handle_;
}
