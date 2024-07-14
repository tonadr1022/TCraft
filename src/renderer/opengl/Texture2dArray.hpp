#pragma once

#include <glm/vec2.hpp>

class Texture2dArray {
 public:
  explicit Texture2dArray(const std::vector<std::vector<uint8_t>>& pixels, const glm::ivec2& dims);
  Texture2dArray(const Texture2dArray& other) = delete;
  Texture2dArray operator=(const Texture2dArray& other) = delete;
  Texture2dArray(Texture2dArray&& other) noexcept;
  Texture2dArray& operator=(Texture2dArray&& other) noexcept;
  ~Texture2dArray();

  [[nodiscard]] uint32_t Id() const { return id_; }
  [[nodiscard]] glm::ivec2 Dims() const { return dims_; }
  void MakeNonResident() const;
  [[nodiscard]] bool IsValid() const;

  void Bind() const;

 private:
  glm::ivec2 dims_{};
  uint32_t id_{0};
};
