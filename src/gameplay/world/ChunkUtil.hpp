#pragma once

#include <glm/fwd.hpp>

namespace util::chunk {

glm::ivec3 WorldToChunkPos(const glm::ivec3& world_pos);
glm::ivec3 WorldToPosInChunk(const glm::ivec3& world_pos);

}  // namespace util::chunk
