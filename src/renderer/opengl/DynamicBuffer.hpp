#pragma once

class DynamicBuffer {
 public:
  DynamicBuffer();
  ~DynamicBuffer();
  void Init(uint32_t size_bytes, uint32_t alignment);
  [[nodiscard]] uint32_t Id() const;

  DynamicBuffer(DynamicBuffer& other) = delete;
  DynamicBuffer& operator=(DynamicBuffer& other) = delete;
  DynamicBuffer(DynamicBuffer&& other) noexcept;
  DynamicBuffer& operator=(DynamicBuffer&& other) noexcept;

  void Bind(uint32_t target) const;
  void BindBase(uint32_t target, uint32_t slot) const;

  // Returns the offset
  [[nodiscard]] uint32_t Allocate(uint32_t size_bytes, void* data, uint32_t& offset);
  void Free(uint32_t handle);
  [[nodiscard]] bool Valid() const;
  [[nodiscard]] uint32_t NumAllocs() const;

 private:
  uint32_t id_{0};
  uint32_t alignment_{0};
  uint32_t next_handle_{1};
  uint32_t num_allocs_{0};

  struct Allocation {
    uint32_t size_bytes;
    uint32_t offset;
    uint32_t handle{0};
  };

  std::vector<Allocation> allocs_;
  using Iterator = decltype(allocs_.begin());
  void Coalesce(Iterator& alloc);
};
