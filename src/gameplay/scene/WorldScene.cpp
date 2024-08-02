#include "WorldScene.hpp"

#include <imgui.h>

#include <nlohmann/json.hpp>

#include "application/SceneManager.hpp"
#include "application/Window.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "gameplay/world/ChunkManager.hpp"
#include "renderer/Renderer.hpp"
#include "resource/Image.hpp"
#include "resource/TextureManager.hpp"
#include "util/JsonUtil.hpp"
#include "util/LoadFile.hpp"
#include "util/Paths.hpp"

WorldScene::WorldScene(SceneManager& scene_manager, std::string_view path)
    : Scene(scene_manager), chunk_manager_(block_db_) {
  ZoneScoped;
  EASSERT_MSG(!path.empty(), "Can't load world scene without a loaded world name");
  {
    ZoneScopedN("Initialize data");
    try {
      auto data = util::LoadJsonFile(std::string(path) + "/data.json");
      auto level_data = util::LoadJsonFile(std::string(path) + "/level.json");
      auto seed = json_util::GetString(level_data, "seed");
      spdlog::info("loading");
      EASSERT_MSG(seed.has_value(), "Missing seed from level.json");
      static std::hash<std::string> hasher;
      seed_ = hasher(seed.value());
      std::array<float, 3> player_pos =
          data.value("player_position", std::array<float, 3>{0, 0, 0});
      player_.SetPosition({player_pos[0], player_pos[1], player_pos[2]});
    } catch (std::runtime_error& error) {
      spdlog::info("failed to load world data");
      scene_manager_.SetNextSceneOnConstructionError("main_menu");
      return;
    }
  }
  player_.Init();
  block_db_.Init();
  {
    ZoneScopedN("Load block mesh data");
    std::unordered_map<std::string, uint32_t> tex_name_to_idx;
    Image image;
    int tex_idx = 0;
    std::vector<void*> all_texture_pixel_data;
    for (const auto& tex_name : block_db_.GetTextureNamesInUse()) {
      util::LoadImage(image, GET_PATH("resources/textures/" + tex_name + ".png"));
      // TODO: handle other sizes/animations
      if (image.width != 32 || image.height != 32) continue;
      tex_name_to_idx[tex_name] = tex_idx++;
      all_texture_pixel_data.emplace_back(image.pixels);
    }
    chunk_render_params_.chunk_tex_array_handle =
        TextureManager::Get().Create2dArray({.all_pixels_data = all_texture_pixel_data,
                                             .dims = glm::ivec2{32, 32},
                                             .generate_mipmaps = true,
                                             .internal_format = GL_RGBA8,
                                             .format = GL_RGBA,
                                             .filter_mode_min = GL_NEAREST_MIPMAP_LINEAR,
                                             .filter_mode_max = GL_NEAREST});
    block_db_.LoadMeshData(tex_name_to_idx);
  }
  chunk_manager_.Init();
}

void WorldScene::Update(double dt) {
  ZoneScoped;
  player_.Update(dt);
}

bool WorldScene::OnEvent(const SDL_Event& event) {
  if (player_.OnEvent(event)) {
    return true;
  }
  switch (event.type) {
    case SDL_KEYDOWN:
      if (event.key.keysym.sym == SDLK_p && event.key.keysym.mod & KMOD_ALT) {
        scene_manager_.LoadScene("main_menu");
        return true;
      }
  }
  return false;
}

void WorldScene::Render() {
  ZoneScoped;
  RenderInfo render_info{.vp_matrix = player_.GetCamera().GetProjection(window_.GetAspectRatio()) *
                                      player_.GetCamera().GetView()};

  {
    ZoneScopedN("Submit chunk draw commands");
    // TODO: only send to renderer the chunks ready to be rendered instead of the whole map
    for (const auto& it : chunk_manager_.GetVisibleChunks()) {
      auto& mesh = it.second->GetMesh();
      glm::vec3 pos = it.first * ChunkLength;
      Renderer::Get().SubmitChunkDrawCommand(glm::translate(glm::mat4{1}, pos), mesh.Handle());
    }
  }
  Renderer::Get().RenderWorld(chunk_render_params_, render_info);
}

WorldScene::~WorldScene() {
  ZoneScoped;
  auto pos = player_.Position();
  std::array<float, 3> player_pos = {pos.x, pos.y, pos.z};
  nlohmann::json j = {{"player_position", player_pos}};
}

void WorldScene::OnImGui() {
  ZoneScoped;
  chunk_manager_.OnImGui();
  player_.OnImGui();
}
