cmake_minimum_required(VERSION 3.21)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/vcpkg.cmake")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(voxels VERSION 1.0)

find_package(SDL2 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(Tracy CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(thread-pool CONFIG REQUIRED)
find_path(BSHOSHANY_THREAD_POOL_INCLUDE_DIRS "BS_thread_pool.hpp")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-nan-infinity-disabled")
set(FASTNOISE2_NOISETOOL OFF CACHE BOOL "Build Noise Tool" FORCE) #example if don't need the graph tool
add_subdirectory(dep/FastNoise2)

add_compile_definitions(TRACY_ENABLE)
option(TRACY_ENABLE "" ON)


add_compile_definitions(SHADER_PATH="${CMAKE_SOURCE_DIR}/resources/shaders/")
add_compile_definitions(TEXTURE_PATH="${CMAKE_SOURCE_DIR}/resources/textures/")
add_compile_definitions(MODEL_PATH="${CMAKE_SOURCE_DIR}/resources/models/")
add_compile_definitions(SRC_PATH="${CMAKE_SOURCE_DIR}/")



include_directories(dep)
include_directories(src)
add_subdirectory(src)

# wayland environment variables nvidia to avoid integrated gpu
# __NV_PRIME_RENDER_OFFLOAD=1
# __GLX_VENDOR_LIBRARY_NAME=nvidia
# DRI_PRIME=1
