#include "DynamicBuffer.hpp"

#include <tracy/Tracy.hpp>

#include "pch.hpp"

DynamicBuffer::DynamicBuffer() = default;

DynamicBuffer::~DynamicBuffer() {
  spdlog::info("dynamic buffer destruction");
  if (id_) {
    glDeleteBuffers(1, &id_);
  }
}

void DynamicBuffer::Init(uint32_t size_bytes, uint32_t alignment) {
  ZoneScoped;
  alignment_ = alignment;
  // align the size
  size_bytes += (alignment_ - (size_bytes % alignment_)) % alignment_;

  glCreateBuffers(1, &id_);
  glNamedBufferStorage(id_, size_bytes, nullptr, GL_DYNAMIC_STORAGE_BIT);

  // create one large free block
  allocs_.push_back({.size_bytes = size_bytes, .offset = 0, .handle = 0});
}

DynamicBuffer& DynamicBuffer::operator=(DynamicBuffer&& other) noexcept {
  if (&other == this) return *this;
  this->~DynamicBuffer();

  id_ = std::exchange(other.id_, 0);
  allocs_ = std::move(other.allocs_);
  alignment_ = other.alignment_;
  next_handle_ = other.next_handle_;

  return *this;
}

DynamicBuffer::DynamicBuffer(DynamicBuffer&& other) noexcept { *this = std::move(other); }

uint32_t DynamicBuffer::Id() const { return id_; }

bool DynamicBuffer::Valid() const { return id_ != 0; }

uint32_t DynamicBuffer::NumAllocs() const { return num_allocs_; }

void DynamicBuffer::Bind(GLuint target) const { glBindBuffer(target, id_); }

void DynamicBuffer::BindBase(uint32_t target, uint32_t slot) const {
  ZoneScoped;
  glBindBufferBase(target, slot, id_);
}

uint32_t DynamicBuffer::Allocate(uint32_t size_bytes, void* data, uint32_t& offset) {
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
  Allocation new_alloc;
  new_alloc.handle = next_handle_++;
  new_alloc.offset = smallest_free_alloc->offset;
  offset = new_alloc.offset;
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

  ++num_allocs_;
  {
    ZoneScopedN("glNamedBufferSubData");
    glNamedBufferSubData(id_, new_alloc.offset, size_bytes, data);
  }
  return new_alloc.handle;
}

void DynamicBuffer::Free(uint32_t handle) {
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

  --num_allocs_;
}

void DynamicBuffer::Coalesce(Iterator& alloc) {
  ZoneScoped;
  EASSERT_MSG(alloc != allocs_.end(), "Don't coalesce a non-existent allocation");

  auto prev_alloc = alloc;
  auto next_alloc = alloc;

  // if no previous or the previous is allocated, can't coalesce previous
  if (prev_alloc != allocs_.begin()) {
    --prev_alloc;
    if (prev_alloc->handle != 0) {
      prev_alloc = allocs_.end();
    }
  } else {
    prev_alloc = allocs_.end();
  }

  // if next alloc doesn't exist or isn't free, can't coalesce it
  ++next_alloc;
  if (next_alloc == allocs_.end() || next_alloc->handle != 0) {
    next_alloc = allocs_.end();
  }

  if (prev_alloc != allocs_.end() && next_alloc != allocs_.end()) {
    prev_alloc->size_bytes += alloc->size_bytes + next_alloc->size_bytes;
    allocs_.erase(alloc, next_alloc);
  } else if (prev_alloc != allocs_.end()) {
    prev_alloc->size_bytes += alloc->size_bytes;
    allocs_.erase(alloc);
  } else if (next_alloc != allocs_.end()) {
    alloc->size_bytes += next_alloc->size_bytes;
    allocs_.erase(next_alloc);
  }
}
