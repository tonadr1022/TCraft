cmake_minimum_required(VERSION 3.21)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(voxels VERSION 1.0)


add_subdirectory(dep)

add_compile_definitions(SHADER_PATH="${CMAKE_SOURCE_DIR}/resources/shaders/")
add_compile_definitions(TEXTURE_PATH="${CMAKE_SOURCE_DIR}/resources/textures/")
add_compile_definitions(MODEL_PATH="${CMAKE_SOURCE_DIR}/resources/models/")
add_compile_definitions(SRC_PATH="${CMAKE_SOURCE_DIR}/")

include_directories(src)
add_subdirectory(src)

# wayland environment variables nvidia to avoid integrated gpu
# __NV_PRIME_RENDER_OFFLOAD=1
# __GLX_VENDOR_LIBRARY_NAME=nvidia
# DRI_PRIME=1
