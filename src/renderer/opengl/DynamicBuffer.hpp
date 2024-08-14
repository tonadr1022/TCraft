#pragma once

struct NoneT {};

/*
 * glBuffer allocator. Can allocate and free blocks. The Allocation struct contains metadata for
 * each block, including user defined data via templating. The current primary use of templating
 * user data is to attach data to a vertex buffer for GPU compute culling.
 */
template <typename UserT = NoneT>
class DynamicBuffer {
 public:
  DynamicBuffer() = default;
  ~DynamicBuffer() {
    ZoneScoped;
    if (id_) {
      glDeleteBuffers(1, &id_);
    }
  }

  void Init(uint32_t size_bytes, uint32_t alignment, size_t max_size = UINT32_MAX) {
    ZoneScoped;
    max_size_ = max_size;
    alignment_ = alignment;
    // align the size
    size_bytes += (alignment_ - (size_bytes % alignment_)) % alignment_;

    glCreateBuffers(1, &id_);
    glNamedBufferStorage(id_, size_bytes, nullptr, GL_DYNAMIC_STORAGE_BIT);

    // create one large free block
    Allocation<UserT> empty_alloc{};
    empty_alloc.size_bytes = size_bytes;
    empty_alloc.offset = 0;
    empty_alloc.handle = 0;
    allocs_.push_back(empty_alloc);
  }

  [[nodiscard]] inline uint32_t Id() const { return id_; }

  DynamicBuffer(DynamicBuffer& other) = delete;
  DynamicBuffer& operator=(DynamicBuffer& other) = delete;
  DynamicBuffer(DynamicBuffer&& other) noexcept {
    if (&other == this) return *this;
    this->~DynamicBuffer();
    id_ = std::exchange(other.id_, 0);
    allocs_ = std::move(other.allocs_);
    alignment_ = other.alignment_;
    next_handle_ = other.next_handle_;
    return *this;
  }

  DynamicBuffer& operator=(DynamicBuffer&& other) noexcept { *this = std::move(other); }

  [[nodiscard]] inline uint32_t AllocSize() const { return sizeof(Allocation<UserT>); }
  [[nodiscard]] const auto& Allocs() const { return allocs_; }

  void Bind(uint32_t target) const { glBindBuffer(target, id_); }
  void BindBase(uint32_t target, uint32_t slot) const { glBindBufferBase(target, slot, id_); }

  // Updates the offset parameter
  [[nodiscard]] uint32_t Allocate(uint32_t size_bytes, void* data, uint32_t& offset,
                                  UserT user_data = {}) {
    ZoneScoped;
    // align the size
    size_bytes += (alignment_ - (size_bytes % alignment_)) % alignment_;
    auto smallest_free_alloc = allocs_.end();
    {
      ZoneScopedN("smallest free alloc");
      // find the smallest free allocation that is large enough
      for (auto it = allocs_.begin(); it != allocs_.end(); it++) {
        // adequate if free and size fits
        if (it->handle == 0 && it->size_bytes >= size_bytes) {
          // if it's the first or it's smaller, set it to the new smallest free alloc
          if (smallest_free_alloc == allocs_.end() ||
              it->size_bytes < smallest_free_alloc->size_bytes) {
            smallest_free_alloc = it;
          }
        }
      }
      // if there isn't an allocation small enough, return 0, null handle
      if (smallest_free_alloc == allocs_.end()) {
        spdlog::error("uh oh, no space left");
        return 0;
      }
    }

    // create new allocation
    Allocation<UserT> new_alloc(user_data);
    new_alloc.handle = next_handle_++;
    new_alloc.offset = smallest_free_alloc->offset;
    new_alloc.size_bytes = size_bytes;

    // update free allocation
    smallest_free_alloc->size_bytes -= size_bytes;
    smallest_free_alloc->offset += size_bytes;

    // if smallest free alloc is now empty, replace it, otherwise insert it
    if (smallest_free_alloc->size_bytes == 0) {
      *smallest_free_alloc = new_alloc;
    } else {
      ZoneScopedN("Insert");
      allocs_.insert(smallest_free_alloc, new_alloc);
    }

    ++num_active_allocs_;
    {
      ZoneScopedN("glNamedBufferSubData");
      glNamedBufferSubData(id_, new_alloc.offset, size_bytes, data);
    }
    offset = new_alloc.offset;
    return new_alloc.handle;
  }
  void Free(uint32_t handle) {
    ZoneScoped;
    auto it = allocs_.end();
    for (it = allocs_.begin(); it != allocs_.end(); it++) {
      if (it->handle == handle) break;
    }
    if (it == allocs_.end()) {
      spdlog::error("Allocation handle not found: {}", handle);
      return;
    }

    it->handle = 0;
    Coalesce(it);

    --num_active_allocs_;
  }
  [[nodiscard]] inline bool Valid() const { return id_ != 0; }
  [[nodiscard]] inline uint32_t NumActiveAllocs() const { return num_active_allocs_; }

  template <typename UT = UserT>
  struct Allocation {
    uint64_t handle{0};
    uint32_t offset{0};
    uint32_t size_bytes{0};
    UT user_data;
    explicit Allocation(UT data = {}) : user_data(data) {}
  };

  template <>
  struct Allocation<void> {
    Allocation() = default;
    uint64_t handle{0};
    uint32_t offset{0};
    uint32_t size_bytes{0};
  };

 private:
  uint32_t id_{0};
  uint32_t alignment_{0};
  uint64_t next_handle_{1};
  uint32_t num_active_allocs_{0};
  size_t max_size_;

  std::vector<Allocation<UserT>> allocs_;
  using Iterator = decltype(allocs_.begin());
  void Coalesce(Iterator& it) {
    ZoneScoped;
    EASSERT_MSG(it != allocs_.end(), "Don't coalesce a non-existent allocation");

    bool remove_it = false;
    bool remove_next = false;

    // merge with next alloc
    if (it != allocs_.end() - 1) {
      auto next = it + 1;
      if (next->handle == 0) {
        it->size_bytes += next->size_bytes;
        remove_next = true;
      }
    }

    // merge with previous alloc
    if (it != allocs_.begin()) {
      auto prev = it - 1;
      if (prev->handle == 0) {
        prev->size_bytes += it->size_bytes;
        remove_it = true;
      }
    }

    // erase merged allocations
    if (remove_it && remove_next)
      allocs_.erase(it, it + 2);  // curr and next
    else if (remove_it)
      allocs_.erase(it);  // only curr
    else if (remove_next)
      allocs_.erase(it + 1);  // only next
  }
};
