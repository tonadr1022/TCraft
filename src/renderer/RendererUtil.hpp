#pragma once

struct BlockData;
struct BlockMeshData;
class Texture;

namespace util::renderer {

extern void RenderAndWriteIcons(const std::vector<BlockData>& block_data,
                                const std::vector<BlockMeshData>& mesh_data,
                                const Texture& tex_arr);
}
