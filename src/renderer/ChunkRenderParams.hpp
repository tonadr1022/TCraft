#pragma once

struct ChunkRenderParams {
  uint32_t chunk_tex_array_handle{0};
  std::string shader_name{"chunk_batch"};
  bool render_chunks_on_{false};
};
