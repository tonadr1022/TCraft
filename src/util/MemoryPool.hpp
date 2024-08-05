#pragma once

template <typename T>
class MemoryPool {
 public:
  explicit MemoryPool(size_t blockCount) : block_count_(blockCount) {
    // Allocate a large block of memory to act as the pool
    memory_ = new char[sizeof(T) * block_count_];
    // Initialize the free list with all blocks
    free_blocks_.reserve(block_count_);
    for (size_t i = 0; i < block_count_; ++i) {
      free_blocks_.push_back(reinterpret_cast<T*>(memory_ + i * sizeof(T)));
    }
  }

  ~MemoryPool() { delete[] memory_; }

  // Allocate a block of memory from the pool
  [[nodiscard]] T* Allocate() {
    EASSERT_MSG(!free_blocks_.empty(), "Memory pool out of memory");
    T* block = free_blocks_.back();
    free_blocks_.pop_back();
    return block;
  }

  // Free a block of memory back to the pool
  void Free(T* block) { free_blocks_.push_back(static_cast<T*>(block)); }

 private:
  size_t block_count_;           // Total number of blocks
  char* memory_;                 // The actual memory pool
  std::vector<T*> free_blocks_;  // Stack of free blocks
};
