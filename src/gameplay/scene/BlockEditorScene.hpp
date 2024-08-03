#pragma once

#include "application/Scene.hpp"
#include "gameplay/Player.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "renderer/ChunkRenderParams.hpp"
#include "renderer/Mesh.hpp"

class TextureMaterial;

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
    Mesh mesh;
    BlockMeshData mesh_data;
  };

  enum class EditMode {
    AddModel,
    EditModel,
    AddBlock,
    EditBlock,
  };

  EditMode edit_mode_{EditMode::AddBlock};
  float block_rot_{0};
  void Reload();
  void HandleAddModelTextureChange(BlockModelType type);
  void HandleEditModelChange();
  void HandleModelChange(BlockModelType type);
  void ResetAddModelData();
  bool add_model_editor_open_{false};

  std::array<SingleBlock, 3> add_model_blocks_;
  BlockModelType add_model_type_{BlockModelType::All};
  BlockModelDataAll add_model_data_all_;
  BlockModelDataTopBot add_model_data_top_bot_;
  BlockModelDataUnique add_model_data_unique_;

  SingleBlock edit_model_block_;
  BlockModelType edit_model_type_{BlockModelType::All};
  BlockModelDataAll edit_model_data_all_;
  BlockModelDataTopBot edit_model_data_top_bot_;
  BlockModelDataUnique edit_model_data_unique_;

  BlockDB block_db_;
  Player player_;

  std::vector<SingleBlock> blocks_;

  std::vector<BlockData> existing_block_data_;
  BlockData add_block_data_;
  std::vector<std::string> all_block_model_names_;
  std::vector<std::string> all_block_texture_names_;
  std::unordered_set<std::string> all_block_model_names_set_;
  std::unordered_map<std::string, uint32_t> tex_name_to_idx_;
  void TexSelectMenu(EditMode mode);

  constexpr const static std::array<std::string, 3> TexTypes = {"all", "top_bottom", "unique"};
  constexpr const static std::array<std::string, 3> TexTypeNames = {"All", "Top Bottom", "Unique"};

  BlockData original_edit_block_data_;
  std::string original_edit_block_model_name_;
  uint32_t edit_block_idx_{1};

  ChunkRenderParams chunk_render_params_;

  std::shared_ptr<TextureMaterial> cross_hair_mat_;
};
