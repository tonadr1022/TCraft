#version 460 core

layout(location = 0) in uvec2 data;

layout(location = 0) out VS_OUT {
    vec3 pos_world_space;
    vec3 tex_coords;
} vs_out;

struct UniformData {
    vec4 pos;
};

layout(std430, binding = 0) readonly buffer uniform_data_buffer {
    UniformData uniforms[];
};

// std140 explicitly states the memory layout.
// https://registry.khronos.org/OpenGL/extensions/ARB/ARB_uniform_buffer_object.txt
layout(std140, binding = 0) uniform Matrices
{
    mat4 vp_matrix;
    vec3 cam_pos;
};

void main() {
    uint x = bitfieldExtract(data.x, 0, 8);
    uint y = bitfieldExtract(data.x, 8, 8);
    uint z = bitfieldExtract(data.x, 16, 8);
    uint u = bitfieldExtract(data.y, 0, 8);
    uint v = bitfieldExtract(data.y, 8, 8);
    uint tex_idx = bitfieldExtract(data.y, 16, 16);
    UniformData uniform_data = uniforms[gl_DrawID + gl_InstanceID];
    vec4 pos_world_space = vec4(vec3(x, y, z) + uniform_data.pos.xyz, 1.0);
    gl_Position = vp_matrix * pos_world_space;
    vs_out.pos_world_space = vec3(pos_world_space);
    vs_out.tex_coords = vec3(u, v, tex_idx);
}
