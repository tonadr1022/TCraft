project(voxels)

set(SOURCES
    application/main.cpp
    application/Application.cpp
    application/Window.cpp
    application/SettingsManager.cpp
    application/EventDispatcher.cpp
    application/SceneManager.cpp
    application/WorldManager.cpp
    application/Scene.cpp

    camera/FPSCamera.cpp
    camera/OrbitCamera.cpp

    renderer/ShaderManager.cpp
    renderer/Renderer.cpp
    renderer/ChunkMesher.cpp
    renderer/Mesh.cpp
    renderer/Frustum.cpp
    renderer/Material.cpp
    renderer/RendererUtil.cpp
    renderer/opengl/Texture2d.cpp
    renderer/opengl/Texture2dArray.cpp
    renderer/opengl/Debug.cpp
    renderer/opengl/Shader.cpp
    renderer/opengl/VertexArray.cpp
    renderer/opengl/Buffer.cpp
    renderer/opengl/DynamicBuffer.cpp

    gameplay/Player.cpp
    gameplay/GamePlayer.cpp
    gameplay/InventoryUI.cpp
    gameplay/scene/WorldScene.cpp
    gameplay/scene/BlockEditorScene.cpp
    gameplay/scene/MainMenuScene.cpp
    gameplay/world/TerrainGenerator.cpp
    gameplay/world/Chunk.cpp
    gameplay/world/ChunkData.cpp
    gameplay/world/BlockDB.cpp
    gameplay/world/ChunkManager.cpp
    gameplay/world/ChunkUtil.cpp
    gameplay/world/Terrain.cpp

    resource/TextureManager.cpp
    resource/MaterialManager.cpp

    util/LoadFile.cpp
    util/StringUtil.cpp
    util/JsonUtil.cpp

    EAssert.cpp
    pch.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
    endif()
endif()
target_precompile_headers(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/pch.hpp)

target_include_directories(${PROJECT_NAME} PRIVATE ${BSHOSHANY_THREAD_POOL_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    imgui::imgui
    GLEW::GLEW
    glm::glm
    Tracy::TracyClient
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    FastNoise
    libcuckoo::libcuckoo
)
