#pragma once

class Texture;

struct TextureMaterialData {
  uint64_t texture_bindless_handle{0};
  uint64_t layer_idx{0};
};

class TextureMaterial {
 public:
  explicit TextureMaterial(const std::shared_ptr<Texture>& tex);
  TextureMaterial();
  ~TextureMaterial();

  TextureMaterial(TextureMaterial& other) = delete;
  TextureMaterial& operator=(TextureMaterial& other) = delete;
  TextureMaterial(TextureMaterial&& other) noexcept;
  TextureMaterial& operator=(TextureMaterial&& other) noexcept;
  Texture& GetTexture();

  [[nodiscard]] uint32_t Handle() const;
  [[nodiscard]] inline bool IsAllocated() const { return handle_ != 0; };

 private:
  std::shared_ptr<Texture> tex_{nullptr};
  uint32_t handle_{0};
};
