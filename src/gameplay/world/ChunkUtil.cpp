#include "ChunkUtil.hpp"

#include <glm/vec3.hpp>

#include "gameplay/world/ChunkDef.hpp"

namespace util::chunk {

glm::ivec3 WorldToChunkPos(const glm::ivec3& world_pos) {
  return glm::ivec3{static_cast<int>(std::floor(static_cast<float>(world_pos.x) / kChunkLength)),
                    static_cast<int>(std::floor(static_cast<float>(world_pos.y) / kChunkLength)),
                    static_cast<int>(std::floor(static_cast<float>(world_pos.z) / kChunkLength))};
}

glm::ivec3 WorldToPosInChunk(const glm::ivec3& world_pos) {
  return world_pos - WorldToChunkPos(world_pos) * kChunkLength;
}

}  // namespace util::chunk
