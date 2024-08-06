#pragma once

#include <glm/vec2.hpp>

struct Texture2dArrayCreateParams {
  const std::vector<void*>& all_pixels_data;
  glm::ivec2 dims;
  bool generate_mipmaps{true};
  uint32_t internal_format;
  uint32_t format;
  uint32_t filter_mode_min;
  uint32_t filter_mode_max;
  uint32_t texture_wrap{0x812F};
};

class Texture2dArray {
 public:
  explicit Texture2dArray(const Texture2dArrayCreateParams& params);
  explicit Texture2dArray(const std::string& param_path,
                          std::unordered_map<std::string, uint32_t>& name_to_idx);
  Texture2dArray() = default;
  Texture2dArray(const Texture2dArray& other) = delete;
  Texture2dArray operator=(const Texture2dArray& other) = delete;
  Texture2dArray(Texture2dArray&& other) noexcept;
  Texture2dArray& operator=(Texture2dArray&& other) noexcept;
  ~Texture2dArray();

  [[nodiscard]] uint32_t Id() const { return id_; }
  [[nodiscard]] glm::ivec2 Dims() const { return dims_; }
  void MakeNonResident() const;
  [[nodiscard]] bool IsValid() const;

  void Bind(int unit) const;

 private:
  void LoadFromParams(const Texture2dArrayCreateParams& params);
  uint32_t id_{0};
  glm::ivec2 dims_{};
};
