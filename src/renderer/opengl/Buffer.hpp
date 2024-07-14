#pragma once

class Buffer {
 public:
  Buffer();
  void Init(uint32_t size_bytes, GLbitfield flags, void* data = nullptr);
  [[nodiscard]] inline uint32_t Id() const { return id_; }

  // Buffer(Buffer& other) = delete;
  // Buffer& operator=(Buffer& other) = delete;
  // Buffer(Buffer&& other) noexcept;
  // Buffer& operator=(Buffer&& other) noexcept;

  /** @brief returns the index of the allocation on the gpu */
  uint32_t SubData(size_t size_bytes, void* data);
  [[nodiscard]] uint32_t NumAllocs() const { return num_allocs_; }
  void Bind(uint32_t target) const;
  void BindBase(uint32_t target, uint32_t slot) const;
  void ResetOffset();
  void* Map(uint32_t access);
  [[nodiscard]] void* MapRange(uint32_t offset, uint32_t length_bytes, GLbitfield access) const;
  void Unmap();

  ~Buffer();
  [[nodiscard]] uint32_t Offset() const { return offset_; }

 private:
  uint32_t num_allocs_{0};
  uint32_t offset_{0};
  uint32_t id_{0};
  bool mapped_{false};
};
