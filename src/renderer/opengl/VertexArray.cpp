#include "VertexArray.hpp"

#include "pch.hpp"

void VertexArray::Bind() const { glBindVertexArray(id_); }

void VertexArray::Init() { glCreateVertexArrays(1, &id_); }

VertexArray::VertexArray() = default;

VertexArray::~VertexArray() {
  if (id_) glDeleteVertexArrays(1, &id_);
}

void VertexArray::AttachVertexBuffer(uint32_t buffer_id, uint32_t binding_index, uint32_t offset,
                                     size_t size) const {
  glVertexArrayVertexBuffer(id_, binding_index, buffer_id, offset, size);
}

void VertexArray::AttachElementBuffer(uint32_t buffer_id) const {
  glVertexArrayElementBuffer(id_, buffer_id);
}

void VertexArray::EnableAttributeInternal(size_t index, size_t size, uint32_t relative_offset,
                                          uint32_t type, bool is_integral) const {
  glEnableVertexArrayAttrib(id_, index);
  if (is_integral) {
    glVertexArrayAttribIFormat(id_, index, size, type, relative_offset);
  } else {
    glVertexArrayAttribFormat(id_, index, size, type, GL_FALSE, relative_offset);
  }
  glVertexArrayAttribBinding(id_, index, 0);
}
