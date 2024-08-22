#pragma once

#include <glm/vec2.hpp>
#include <string>

#include "resource/Image.hpp"

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
struct TextureCubeCreateParams {
  const std::string& path;
};

struct TextureCubeCreateParamsPaths {
  std::array<std::string, 6> paths;
};

struct Texture2DArrayCreateParams {
  const std::vector<Image>& images;
  bool generate_mipmaps{true};
  uint32_t internal_format;
  uint32_t format;
  uint32_t filter_mode_min{GL_LINEAR};
  uint32_t filter_mode_max{GL_LINEAR};
  uint32_t texture_wrap{GL_CLAMP_TO_EDGE};
  bool bindless{false};
};

class Texture {
 public:
  explicit Texture(const Texture2DCreateParamsEmpty& params);
  explicit Texture(const Texture2DCreateParams& params);
  explicit Texture(const Texture2DArrayCreateParams& params);
  // explicit Texture(const TextureCubeCreateParams& params);
  explicit Texture(const TextureCubeCreateParamsPaths& params);
  Texture(const Texture& other) = delete;
  Texture operator=(const Texture& other) = delete;
  Texture(Texture&& other) noexcept;
  Texture& operator=(Texture&& other) noexcept;
  ~Texture();
  [[nodiscard]] uint32_t Id() const { return id_; }
  [[nodiscard]] glm::ivec2 Dims() const { return dims_; }
  [[nodiscard]] uint64_t BindlessHandle() const { return bindless_handle_; }
  void MakeNonResident();
  void MakeResident();
  [[nodiscard]] bool IsValid() const;

  void Bind() const;
  void Bind(int unit) const;

 private:
  uint32_t id_{0};
  uint32_t bindless_handle_{0};
  bool resident_{false};
  glm::ivec2 dims_{};
};
