#include "ChunkUtil.hpp"

#include <glm/vec3.hpp>

#include "gameplay/world/ChunkData.hpp"

namespace util::chunk {

glm::vec3 WorldToChunkPos(const glm::ivec3& world_pos) { return world_pos / ChunkLength; }

glm::vec3 WorldToPosInChunk(const glm::ivec3& world_pos) { return world_pos % ChunkLength; }

}  // namespace util::chunk
