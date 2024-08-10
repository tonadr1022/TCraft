#pragma once

#include <glm/vec2.hpp>
#include <string>

struct Texture2DCreateParams {
  std::string filepath;
  uint32_t internal_format{GL_RGBA8};
  uint32_t filter_mode_min{GL_LINEAR};
  uint32_t filter_mode_max{GL_LINEAR};
  bool generate_mipmaps{true};
  bool bindless{true};
  bool flip{true};
};

struct Texture2DCreateParamsEmpty {
  uint32_t width;
  uint32_t height;
  uint32_t filter_mode_min{GL_NEAREST};
  uint32_t filter_mode_max{GL_NEAREST};
  bool bindless{true};
  bool generate_mipmaps{false};
};

class Texture2D {
 public:
  explicit Texture2D(const Texture2DCreateParamsEmpty& params);
  explicit Texture2D(const Texture2DCreateParams& params);
  Texture2D(const Texture2D& other) = delete;
  Texture2D operator=(const Texture2D& other) = delete;
  Texture2D(Texture2D&& other) noexcept;
  Texture2D& operator=(Texture2D&& other) noexcept;
  ~Texture2D();
  [[nodiscard]] uint32_t Id() const { return id_; }
  [[nodiscard]] glm::ivec2 Dims() const { return dims_; }
  [[nodiscard]] uint64_t BindlessHandle() const { return bindless_handle_; }
  void MakeNonResident();
  void MakeResident();
  [[nodiscard]] bool IsValid() const;

  void Bind() const;

 private:
  uint32_t id_{0};
  uint32_t bindless_handle_{0};
  bool resident_{false};
  glm::ivec2 dims_{};
};
