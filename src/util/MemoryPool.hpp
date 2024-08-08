#pragma once

template <typename T>
class MemoryPool {
 public:
  explicit MemoryPool(size_t blockCount) : block_count_(blockCount) {
    memory_ = new char[sizeof(T) * block_count_];
    free_blocks_.reserve(block_count_);
    for (size_t i = 0; i < block_count_; ++i) {
      free_blocks_.push_back(reinterpret_cast<T*>(memory_ + i * sizeof(T)));
    }
  }

  ~MemoryPool() { delete[] memory_; }

  [[nodiscard]] T* Allocate() {
    EASSERT_MSG(!free_blocks_.empty(), "Memory pool out of memory");
    T* block = free_blocks_.back();
    free_blocks_.pop_back();
    return block;
  }

  void Free(T* block) { free_blocks_.push_back(static_cast<T*>(block)); }

 private:
  size_t block_count_;
  char* memory_;
  std::vector<T*> free_blocks_;
};
