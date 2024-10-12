#include "ShadowScene.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/trigonometric.hpp>
#include <random>

#include "application/Window.hpp"
#include "gameplay/world/Chunk.hpp"
#include "gameplay/world/TerrainGenerator.hpp"
#include "renderer/ChunkMesher.hpp"
#include "renderer/Renderer.hpp"
#include "renderer/opengl/Shader.hpp"
#include "renderer/opengl/Texture2d.hpp"
#include "resource/Image.hpp"
#include "resource/TextureManager.hpp"
#include "util/LoadFile.hpp"
#include "util/Paths.hpp"

namespace {

std::random_device device;
std::mt19937 generator = std::mt19937(device());
uint32_t mesh_handle = 0;

}  // namespace
void ShadowScene::render_plane() {
  if (plane_vao.Id() == 0) {
    // clang-format off
    float plane_vertices[] = {
        // positions            // normals         // texcoords
        25.0f, -2.0f, 25.0f, 0.0f,  1.0f,   0.0f,  25.0f,  0.0f, 
      -25.0f, -2.0f, 25.0f,  0.0f, 1.0f,  0.0f,  0.0f,  0.0f,  
      -25.0f, -2.0f, -25.0f, 0.0f, 1.0f,   0.0f,  0.0f,   25.0f,
        25.0f, -2.0f, 25.0f, 0.0f,  1.0f,   0.0f,  25.0f,  0.0f, 
      -25.0f, -2.0f, -25.0f, 0.0f, 1.0f,  0.0f,  0.0f,  25.0f, 
      25.0f,  -2.0f, -25.0f, 0.0f, 1.0f,   0.0f,  25.0f,  25.0f
    };
    // clang-format on
    // plane VAO
    plane_vao.Init();
    plane_vbo.Init(sizeof(float) * 8 * 8, 0, plane_vertices);
    // link vertex attributes
    plane_vao.Bind();
    plane_vao.EnableAttribute<float>(0, 3, 0);
    plane_vao.EnableAttribute<float>(1, 3, sizeof(float) * 3);
    plane_vao.EnableAttribute<float>(2, 2, sizeof(float) * 6);
    plane_vao.AttachVertexBuffer(plane_vbo.Id(), 0, 0, sizeof(float) * 8);
  }
  plane_vao.Bind();
  glDrawArrays(GL_TRIANGLES, 0, 6);
}

void ShadowScene::render_cube() {
  // initialize (if necessary)
  if (cube_vao.Id() == 0) {
    float vertices[] = {
        // back face
        -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,  // bottom-left
        1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,    // top-right
        1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,   // bottom-right
        1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,    // top-right
        -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,  // bottom-left
        -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,   // top-left
        // front face
        -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  // bottom-left
        1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,   // bottom-right
        1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,    // top-right
        1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,    // top-right
        -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,   // top-left
        -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  // bottom-left
        // left face
        -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,    // top-right
        -1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,   // top-left
        -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // bottom-left
        -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // bottom-left
        -1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,   // bottom-right
        -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,    // top-right
                                                             // right face
        1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,      // top-left
        1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,    // bottom-right
        1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,     // top-right
        1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,    // bottom-right
        1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,      // top-left
        1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,     // bottom-left
        // bottom face
        -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,  // top-right
        1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,   // top-left
        1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,    // bottom-left
        1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,    // bottom-left
        -1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,   // bottom-right
        -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,  // top-right
        // top face
        -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // top-left
        1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,    // bottom-right
        1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,   // top-right
        1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,    // bottom-right
        -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // top-left
        -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f    // bottom-left
    };
    cube_vao.Init();
    cube_vbo.Init(sizeof(float) * 8 * 36, 0, vertices);
    // link vertex attributes
    cube_vao.Bind();
    cube_vao.EnableAttribute<float>(0, 3, 0);
    cube_vao.EnableAttribute<float>(1, 3, sizeof(float) * 3);
    cube_vao.EnableAttribute<float>(2, 2, sizeof(float) * 6);
    cube_vao.AttachVertexBuffer(cube_vbo.Id(), 0, 0, sizeof(float) * 8);
  }
  // render Cube
  cube_vao.Bind();
  // glBindVertexArray(cube_vao);
  glDrawArrays(GL_TRIANGLES, 0, 36);
}

ShadowScene::ShadowScene(SceneManager& scene_manager) : Scene(scene_manager) {
  player_.SetCameraFocused(false);
  player_.SetPosition({0, 0, 0});
  player_.LookAt({0.5f, 0.5f, 0.5f});

  std::unordered_map<std::string, uint32_t> tex_name_to_idx;
  Image image;
  int tex_idx = 0;
  std::vector<Image> images;
  for (const auto& tex_name : block_db_.GetTextureNamesInUse()) {
    util::LoadImage(image, GET_PATH("resources/textures/" + tex_name + ".png"), 4, true);
    // TODO: handle other sizes/animations
    if (image.width != 32 || image.height != 32) continue;
    tex_name_to_idx[tex_name] = tex_idx++;
    images.emplace_back(image);
  }
  chunk_tex_array_ = TextureManager::Get().Load(
      Texture2DArrayCreateParams{.images = images,
                                 .generate_mipmaps = true,
                                 .internal_format = GL_RGBA8,
                                 .format = GL_RGBA,
                                 .filter_mode_min = GL_NEAREST_MIPMAP_LINEAR,
                                 .filter_mode_max = GL_NEAREST,
                                 .texture_wrap = GL_REPEAT});
  Renderer::Get().chunk_tex_array = chunk_tex_array_;
  block_db_.LoadMeshData(tex_name_to_idx, images);
  for (auto const& p : images) {
    util::FreeImage(p.pixels);
  }

  {
    Chunk chunk(glm::vec3(0));
    SingleChunkTerrainGenerator gen(chunk.data, glm::ivec3{0, 0, 0}, 0, terrain_);
    gen.GenerateYLayer(0, block_db_.GetBlockData("stone")->id);
    ChunkMesher mesher(block_db_.GetBlockData(), block_db_.GetMeshData());
    MeshVerticesIndices out_data;
    mesher.GenerateGreedy(chunk, out_data);
    mesh_handle = Renderer::Get().AllocateChunk(out_data.opaque_vertices, out_data.opaque_indices);
  }
}

ShadowScene::~ShadowScene() = default;

void ShadowScene::OnImGui() {}

void ShadowScene::Update(double dt) { player_.Update(dt); }

bool ShadowScene::OnEvent(const SDL_Event& event) { return player_.OnEvent(event); }

void ShadowScene::Render() {
  auto draw_scene = [this](Shader& shader) {
    static std::vector<glm::mat4> model_matrices;
    if (model_matrices.size() == 0) {
      for (int i = 0; i < 10; ++i) {
        static std::uniform_real_distribution<float> offset_distribution =
            std::uniform_real_distribution<float>(-10, 10);
        static std::uniform_real_distribution<float> scale_distribution =
            std::uniform_real_distribution<float>(1.0, 2.0);
        static std::uniform_real_distribution<float> rotation_distribution =
            std::uniform_real_distribution<float>(0, 180);

        auto model = glm::mat4(1.0f);
        model = glm::translate(
            model, glm::vec3(offset_distribution(generator), offset_distribution(generator) + 10.0f,
                             offset_distribution(generator)));
        model = glm::rotate(model, glm::radians(rotation_distribution(generator)),
                            glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
        model = glm::scale(model, glm::vec3(scale_distribution(generator)));
        model_matrices.push_back(model);
      }
    }

    for (const auto& model : model_matrices) {
      shader.SetMat4("model", model);
      render_cube();
    }

    // auto model = glm::mat4(1.0f);
    // shader.SetMat4("model", model);
    // render_plane();
  };

  Renderer::Get().SubmitChunkDrawCommand(glm::translate(glm::mat4(1), glm::vec3(-16, -4, -16)),
                                         mesh_handle);
  Renderer::Get().draw_func = draw_scene;
  auto& camera = player_.GetCamera();
  glm::mat4 proj = camera.GetProjection(window_.GetAspectRatio());
  glm::mat4 view = camera.GetView();
  Renderer::Get().Render(RenderInfo{.vp_matrix = proj * view,
                                    .view_matrix = view,
                                    .proj_matrix = proj,
                                    .view_pos = player_.Position(),
                                    .view_dir = camera.GetFront(),
                                    .camera_near_plane = camera.NearPlane(),
                                    .camera_far_plane = camera.FarPlane()});
}
