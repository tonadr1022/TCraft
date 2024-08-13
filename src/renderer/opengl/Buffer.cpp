#include "Buffer.hpp"

#include "pch.hpp"

Buffer::Buffer() = default;

Buffer::~Buffer() {
  ZoneScoped;
  EASSERT_MSG(!mapped_, "buffer can't be mapped on deletion");
  if (id_) {
    glDeleteBuffers(1, &id_);
  }
}

void Buffer::Init(uint32_t size_bytes, GLbitfield flags, const void* data) {
  if (id_) {
    glDeleteBuffers(1, &id_);
  }
  glCreateBuffers(1, &id_);
  glNamedBufferStorage(id_, size_bytes, data, flags);
}

Buffer::Buffer(Buffer&& other) noexcept { *this = std::move(other); }

Buffer& Buffer::operator=(Buffer&& other) noexcept {
  if (&other == this) return *this;
  this->~Buffer();
  id_ = std::exchange(other.id_, 0);
  offset_ = std::exchange(other.offset_, 0);
  return *this;
}

void Buffer::Bind(GLuint target) const { glBindBuffer(target, id_); }

void Buffer::BindBase(GLuint target, GLuint slot) const { glBindBufferBase(target, slot, id_); }

void Buffer::SubData(size_t size_bytes, void* data) {
  glNamedBufferSubData(id_, offset_, size_bytes, data);
  offset_ += size_bytes;
}

void* Buffer::Map(uint32_t access) {
  mapped_ = true;
  return glMapNamedBuffer(id_, access);
}

void Buffer::ResetOffset() { offset_ = 0; }
void Buffer::SetOffset(uint32_t offset) { offset_ = offset; }

void* Buffer::MapRange(uint32_t offset, uint32_t length_bytes, GLbitfield access) const {
  return glMapNamedBufferRange(id_, offset, length_bytes, access);
}

void Buffer::Unmap() {
  mapped_ = false;
  glUnmapNamedBuffer(id_);
}
