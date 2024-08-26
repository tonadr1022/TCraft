#pragma once

struct BlockData;
struct BlockMeshData;
class Texture;
struct Image;
class BlockDB;
class TextureMaterial;
struct SquareTextureAtlas;
enum class TransparencyType;

namespace util::renderer {
extern void RenderAndWriteIcons(const std::vector<BlockData>& block_data,
                                const std::vector<BlockMeshData>& mesh_data, const Texture& tex_arr,
                                bool rewrite);
extern void RenderAndWriteIcon(const std::string& path, const BlockMeshData& mesh_data,
                               const Texture& tex_arr);
extern void LoadIcons(std::vector<Image>& images);
extern TransparencyType LoadImageAndCheckHasTransparency(const std::string& path,
                                                         int required_channels = 0);
extern SquareTextureAtlas LoadIconTextureAtlas(const std::string& tex_name, const BlockDB& block_db,
                                               const Texture& tex_arr);

}  // namespace util::renderer
