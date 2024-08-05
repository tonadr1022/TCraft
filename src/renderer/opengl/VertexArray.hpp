#pragma once

class VertexArray {
 public:
  VertexArray();
  void Init();
  ~VertexArray();
  void Bind() const;

  [[nodiscard]] inline uint32_t Id() const { return id_; }
  void AttachVertexBuffer(uint32_t buffer_id, uint32_t binding_index, uint32_t offset,
                          size_t size) const;
  void AttachElementBuffer(uint32_t buffer_id) const;

  template <typename T>
  void EnableAttribute(size_t index, size_t size, uint32_t relative_offset) {
    uint32_t type;
    if constexpr (std::is_floating_point_v<T>) {
      type = GL_FLOAT;
    } else if constexpr (std::is_integral_v<T>) {
      if constexpr (std::is_signed_v<T>) {
        type = GL_INT;
      } else {
        type = GL_UNSIGNED_INT;
      }
    }
    EnableAttributeInternal(index, size, relative_offset, type, !std::is_floating_point_v<T>);
  }

 private:
  uint32_t id_{0};
  void EnableAttributeInternal(size_t index, size_t size, uint32_t relative_offset, uint32_t type,
                               bool is_integral) const;
};
