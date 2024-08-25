#pragma once

#include <imgui.h>

#include <glm/ext/vector_float2.hpp>

class TextureMaterial;

struct SquareTextureAtlas {
  glm::vec2 dims;
  glm::vec2 image_dims;
  std::unordered_map<uint32_t, glm::vec2> id_to_offset_map;
  std::shared_ptr<TextureMaterial> material;
  inline void ComputeUVs(uint32_t id, ImVec2& uv0, ImVec2& uv1) const {
    auto it = id_to_offset_map.find(id);
    if (it == id_to_offset_map.end()) return;
    uv0.x = static_cast<float>(it->second.x) / dims.x;
    uv1.y = static_cast<float>(it->second.y) / dims.y;
    uv1.x = static_cast<float>(it->second.x + image_dims.x) / dims.x;
    uv0.y = static_cast<float>(it->second.y + image_dims.y) / dims.y;
  }
};
