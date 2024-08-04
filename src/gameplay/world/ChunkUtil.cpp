#include "ChunkUtil.hpp"

#include <glm/vec3.hpp>

#include "gameplay/world/ChunkData.hpp"

namespace util::chunk {

glm::ivec3 WorldToChunkPos(const glm::ivec3& world_pos) {
  return glm::ivec3{static_cast<int>(std::floor(static_cast<float>(world_pos.x) / ChunkLength)),
                    static_cast<int>(std::floor(static_cast<float>(world_pos.y) / ChunkLength)),
                    static_cast<int>(std::floor(static_cast<float>(world_pos.z) / ChunkLength))};
}

glm::ivec3 WorldToPosInChunk(const glm::ivec3& world_pos) {
  return world_pos - WorldToChunkPos(world_pos) * ChunkLength;
}

}  // namespace util::chunk
