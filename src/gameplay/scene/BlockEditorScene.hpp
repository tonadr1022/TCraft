#pragma once

#include "application/Scene.hpp"
#include "gameplay/Player.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "gameplay/world/Terrain.hpp"
#include "renderer/TextureAtlas.hpp"

class Texture;
class TextureMaterial;
namespace detail {
struct BlockEditorState;
}

class BlockEditorScene : public Scene {
 public:
  explicit BlockEditorScene(SceneManager& scene_manager);
  ~BlockEditorScene() override;

  void OnImGui() override;
  void Update(double dt) override;
  bool OnEvent(const SDL_Event& event) override;
  void Render() override;

 private:
  struct SingleBlock {
    glm::vec3 pos;
    uint32_t mesh_handle{0};
    BlockMeshData mesh_data;
  };

  enum class EditMode {
    kAddModel,
    kEditModel,
    kAddBlock,
    kEditBlock,
    kEditTerrain,
  };

  EditMode edit_mode_{EditMode::kAddBlock};
  float block_rot_{0};
  void Reload();
  void HandleAddModelTextureChange(BlockModelType type);
  void HandleEditModelChange();
  void SetModelTexIndices(std::array<uint32_t, 6>& indices, const std::string& model_name);
  void HandleModelChange(BlockModelType type);
  void ResetAddModelData();
  void SetAddBlockModelData();
  void ImGuiTerrainEdit();
  bool first_edit_{true};
  bool add_model_editor_open_{false};

  std::array<SingleBlock, 3> add_model_blocks_;
  BlockModelType add_model_type_{BlockModelType::kAll};
  BlockModelDataAll add_model_data_all_;
  BlockModelDataTopBot add_model_data_top_bot_;
  BlockModelDataUnique add_model_data_unique_;

  SingleBlock edit_model_block_;
  SingleBlock add_block_block_;
  BlockModelType edit_model_type_{BlockModelType::kAll};
  BlockModelDataAll edit_model_data_all_;
  BlockModelDataTopBot edit_model_data_top_bot_;
  BlockModelDataUnique edit_model_data_unique_;

  BlockDB block_db_;
  Player player_;

  std::vector<SingleBlock> blocks_;

  std::vector<BlockData> existing_block_data_;
  std::vector<std::string> all_block_model_names_;
  std::vector<std::string> all_block_texture_names_;
  std::unordered_set<std::string> all_block_model_names_set_;
  std::unordered_map<std::string, uint32_t> tex_name_to_idx_;
  void TexSelectMenu(EditMode mode);

  Terrain terrain_;

  constexpr const static std::array<std::string, 3> kTexTypes = {"all", "top_bottom", "unique"};
  constexpr const static std::array<std::string, 3> kTexTypeNames = {"All", "Top Bottom", "Unique"};

  BlockData original_edit_block_data_;
  std::string original_edit_block_model_name_;
  uint32_t edit_block_idx_{1};

  std::shared_ptr<Texture> chunk_tex_array_{nullptr};

  std::shared_ptr<TextureMaterial> cross_hair_mat_;
  std::unique_ptr<detail::BlockEditorState> state_{nullptr};
  SquareTextureAtlas icon_texture_atlas_;
};
