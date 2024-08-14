#pragma once

class Buffer {
 public:
  Buffer();
  void Init(uint32_t size_bytes, GLbitfield flags, const void* data = nullptr);
  [[nodiscard]] inline uint32_t Id() const { return id_; }

  Buffer(Buffer& other) = delete;
  Buffer& operator=(Buffer& other) = delete;
  Buffer(Buffer&& other) noexcept;
  Buffer& operator=(Buffer&& other) noexcept;

  void SubData(size_t size_bytes, void* data);
  void SubDataStart(size_t size_bytes, void* data);
  void Bind(uint32_t target) const;
  void BindBase(uint32_t target, uint32_t slot) const;
  void ResetOffset();
  void SetOffset(uint32_t offset);
  void* Map(uint32_t access);
  [[nodiscard]] void* MapRange(uint32_t offset, uint32_t length_bytes, GLbitfield access) const;
  void Unmap();

  ~Buffer();
  [[nodiscard]] uint32_t Offset() const { return offset_; }

 private:
  uint32_t offset_{0};
  uint32_t id_{0};
  bool mapped_{false};
};
