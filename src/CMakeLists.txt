project(voxels)

set(SOURCES
    application/main.cpp
    application/Application.cpp
    application/Window.cpp
    application/Settings.cpp
    application/EventDispatcher.cpp
    application/SceneManager.cpp
    application/WorldManager.cpp

    renderer/ShaderManager.cpp
    renderer/Renderer.cpp
    renderer/FPSCamera.cpp
    renderer/ChunkMesher.cpp
    renderer/ChunkMesh.cpp

    renderer/opengl/Texture2d.cpp
    renderer/opengl/Texture2dArray.cpp
    renderer/opengl/Debug.cpp
    renderer/opengl/Shader.cpp
    renderer/opengl/VertexArray.cpp
    renderer/opengl/Buffer.cpp

    util/LoadFile.cpp
    util/StringUtil.cpp
    util/JsonUtil.cpp
    gameplay/Player.cpp
    gameplay/world/TerrainGenerator.cpp
    gameplay/world/Chunk.cpp
    gameplay/world/ChunkData.cpp
    gameplay/world/BlockDB.cpp
    gameplay/scene/WorldScene.cpp
    gameplay/scene/MainMenuScene.cpp

    resource/TextureManager.cpp

    EAssert.cpp
    pch.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_precompile_headers(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/pch.hpp)

target_link_libraries(${PROJECT_NAME} PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    imgui::imgui
    GLEW::GLEW
    glm::glm
    Tracy::TracyClient
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)
