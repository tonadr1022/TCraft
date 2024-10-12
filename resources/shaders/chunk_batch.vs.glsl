#version 460 core

#include "common.glsl"

layout(location = 0) in uvec2 data;

layout(location = 0) out VS_OUT {
    vec3 pos_world_space;
    vec3 tex_coords;
    vec3 color;
    vec3 normal;
} vs_out;

struct UniformData {
    vec4 pos;
};

layout(std430, binding = 0) readonly buffer uniform_data_buffer {
    UniformData uniforms[];
};

layout(std140, binding = 0) uniform Matrices
{
    mat4 vp_matrix;
    mat4 view_matrix;
    vec3 cam_pos;
};

uniform bool u_UseAO = true;

void main() {
    uint x = bitfieldExtract(data.x, 0, 6);
    uint y = bitfieldExtract(data.x, 6, 6);
    uint z = bitfieldExtract(data.x, 12, 6);
    uint u = bitfieldExtract(data.x, 20, 6);
    uint v = bitfieldExtract(data.x, 26, 6);
    uint tex_idx = bitfieldExtract(data.y, 0, 29);
    UniformData uniform_data = uniforms[gl_DrawID + gl_InstanceID];
    vec4 pos_world_space = vec4(vec3(x, y, z) + uniform_data.pos.xyz, 1.0);
    vs_out.normal = CubeNormals[bitfieldExtract(data.y, 29, 3)];
    gl_Position = vp_matrix * pos_world_space;
    vs_out.pos_world_space = vec3(pos_world_space);
    vs_out.tex_coords = vec3(u, v, tex_idx);
    if (u_UseAO) {
        vs_out.color = vec3(AOcurve[bitfieldExtract(data.x, 18, 2)]);
    } else {
        vs_out.color = vec3(1);
    }
}
