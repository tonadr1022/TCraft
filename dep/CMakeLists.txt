find_package(OpenGL REQUIRED)

include(FetchContent)

find_path(BSHOSHANY_THREAD_POOL_INCLUDE_DIRS "BS_thread_pool.hpp")

find_package(SDL2)
if (NOT SDL2_FOUND)
    FetchContent_Declare(
    fetch_sdl2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        c98c4fbff6d8f3016a3ce6685bf8f43433c3efcc
)
    FetchContent_MakeAvailable(fetch_sdl2)
endif()

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.15.0
)
FetchContent_MakeAvailable(spdlog)

# TODO: configurable
add_compile_definitions(TRACY_ENABLE)
option(TRACY_ENABLE "" ON)
FetchContent_Declare(
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG v0.11.1
)
FetchContent_MakeAvailable(tracy)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG master
)

FetchContent_MakeAvailable(imgui)
set(IMGUI_BACKEND_SRC
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
)

set(IMGUI_BACKEND_HEADERS
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.h
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.h
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.h
)

# Include ImGui and backends
add_library(imgui_backend STATIC
    ${IMGUI_BACKEND_SRC}
    ${IMGUI_BACKEND_HEADERS}
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
)

target_include_directories(imgui_backend PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${imgui_SOURCE_DIR}/misc/cpp
)

# Link libraries
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

target_link_libraries(imgui_backend PUBLIC
    SDL2::SDL2
    OpenGL::GL
)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)


add_subdirectory(libcuckoo)
target_include_directories(libcuckoo SYSTEM INTERFACE
    $<BUILD_INTERFACE:${libcuckoo_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

set(FASTNOISE2_NOISETOOL OFF CACHE BOOL "Build Noise Tool" FORCE)
FetchContent_Declare(
    FastNoise2
    GIT_REPOSITORY https://github.com/Auburn/FastNoise2.git
    GIT_TAG v0.10.0-alpha
)
FetchContent_MakeAvailable(FastNoise2)

add_library(bs_thread_pool INTERFACE)

target_include_directories(bs_thread_pool SYSTEM INTERFACE bs_thread_pool)

add_library(glad STATIC glad/src/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/glad/include)
add_library(stb_image INTERFACE)
target_include_directories(stb_image SYSTEM INTERFACE stb_image)
