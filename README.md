# TCraft

TCraft is a rewrite of my
[previous voxel engine](https://github.com/tonadr1022/VoxelEngine3D) in OpenGL
4.6 and C++. This iteration uses AZDO (Approaching Zero Driver Overhead)
techniques to reduce driver overhead, particularly
[glMultidrawElementsIndirect](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glMultiDrawElementsIndirect.xhtml)
to draw all objects that use the same vertex format and shader with a single
draw call. In general, my first iteration was quite messy, as it was one of my
first real attempts at writing C++ and OpenGL, so a common strategy was find
what other people did online and mimic it. This time, though, I actually know
the language (not really, C++ is a lifelong journey).

## Demos
[Demo 1](https://youtu.be/nuAlO2GmP_g)

## Running Locally

- Clone the repository rescursively and/or run `git submodule update` after
  cloning to add the FastNoise2 submodule.
- This project uses Vcpkg to manage dependencies and will fetch Vcpkg and build
  dependencies using it if Vcpkg is not found on your system.
- Use CMake commands, an IDE, or run `python3 tasks.py -cbr` to configure, build
  and run. Pass individual flags to run only that part of the build process

## Features

- Multithreaded terrain generation and meshing
- Block editor to change and add block models and blocks
- World creation
- Block breaking and placing
- Baked ambient occlusion
- Efficient greedy meshing
- Orbit and FPS Camera

## TODO

- Sky
- Configurable terrain generation
- Better block editor GUI and experience
- Multithread texture loading
- Chunk serialization
- Better shading and lighting (baked lighting vs forward+ vs deferred?)

## Dependencies

- [GLEW](https://github.com/nigels-com/glew) - OpenGL extension wrangler
- [GLM](https://github.com/g-truc/glm) - vector math
- [Tracy](https://github.com/wolfpld/tracy) - Profiling
- [Spdlog](https://github.com/gabime/spdlog) - Logging
- [SDL2](https://github.com/libsdl-org/SDL) - Window/input handling
- [Nlohmann-json](https://github.com/nlohmann/json)- JSON parse and write
- [Bshoshany-thread-pool](https://github.com/bshoshany/thread-pool) - (I'll
  learn how to make my own after OS this fall)
- [ImGui](https://github.com/ocornut/imgui) - GUI
