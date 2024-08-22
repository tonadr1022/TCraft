#pragma once

#include <glm/fwd.hpp>

struct BlockData;
struct BlockMeshData;
class Texture;
struct Image;
class BlockDB;
class TextureMaterial;
struct SquareTextureAtlas;

namespace util::renderer {
extern void RenderAndWriteIcons(const std::vector<BlockData>& block_data,
                                const std::vector<BlockMeshData>& mesh_data,
                                const Texture& tex_arr);
extern void RenderAndWriteIcon(const std::string& path, const BlockMeshData& mesh_data,
                               const Texture& tex_arr);
extern void LoadIcons(std::vector<Image>& images);
extern SquareTextureAtlas LoadIconTextureAtlas(const BlockDB& block_db, const Texture& tex_arr);

}  // namespace util::renderer
