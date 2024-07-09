project(voxels)

set(SOURCES
    application/main.cpp
    application/Application.cpp
    application/Window.cpp
    application/Input.cpp
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
    spdlog::spdlog
)
