#pragma once

class Texture2D;

struct TextureMaterialData {
  uint64_t texture_handle{0};
};

class TextureMaterial {
 public:
  TextureMaterial(TextureMaterialData& data, std::shared_ptr<Texture2D> tex);
  TextureMaterial();
  ~TextureMaterial();

  TextureMaterial(TextureMaterial& other) = delete;
  TextureMaterial& operator=(TextureMaterial& other) = delete;
  TextureMaterial(TextureMaterial&& other) noexcept;
  TextureMaterial& operator=(TextureMaterial&& other) noexcept;
  Texture2D& GetTexture();

  [[nodiscard]] uint32_t Handle() const;
  [[nodiscard]] inline bool IsAllocated() const { return handle_ != 0; };

 private:
  std::shared_ptr<Texture2D> tex_{nullptr};
  uint32_t handle_{0};
};
