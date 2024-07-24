#pragma once

#include <glm/fwd.hpp>

namespace util::chunk {

glm::vec3 WorldToChunkPos(const glm::ivec3& world_pos);
glm::vec3 WorldToPosInChunk(const glm::ivec3& world_pos);

}  // namespace util::chunk
