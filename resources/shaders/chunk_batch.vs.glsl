#version 460 core

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

// std140 explicitly states the memory layout.
// https://registry.khronos.org/OpenGL/extensions/ARB/ARB_uniform_buffer_object.txt
layout(std140, binding = 0) uniform Matrices
{
    mat4 vp_matrix;
    mat4 view_matrix;
    vec3 cam_pos;
};

uniform bool u_UseAO = true;

const float AOcurve[4] = float[4](0.55, 0.75, 0.87, 1.0);
//const float AOcurve[4] = float[4](0.3, 0.5, 0.7, 1.0);

const vec3 CubeNormals[6] = vec3[6](
        vec3(1.0, 0.0, 0.0),
        vec3(-1.0, 0.0, 0.0),
        vec3(0.0, 1.0, 0.0),
        vec3(0.0, -1.0, 0.0),
        vec3(0.0, 0.0, 1.0),
        vec3(0.0, 0.0, -1.0)
    );

mat4 BuildTranslation(vec3 delta) {
    return mat4(
        vec4(1.0, 0.0, 0.0, 0.0),
        vec4(0.0, 1.0, 0.0, 0.0),
        vec4(0.0, 0.0, 1.0, 0.0),
        vec4(delta, 1.0)
    );
}

void main() {
    uint x = bitfieldExtract(data.x, 0, 6);
    uint y = bitfieldExtract(data.x, 6, 6);
    uint z = bitfieldExtract(data.x, 12, 6);
    uint u = bitfieldExtract(data.x, 20, 6);
    uint v = bitfieldExtract(data.x, 26, 6);
    uint tex_idx = bitfieldExtract(data.y, 0, 29);
    UniformData uniform_data = uniforms[gl_DrawID + gl_InstanceID];
    vec4 pos_world_space = vec4(vec3(x, y, z) + uniform_data.pos.xyz, 1.0);
    vs_out.normal = transpose(inverse(mat3(BuildTranslation(vec3(pos_world_space.xyz))))) * CubeNormals[bitfieldExtract(data.y, 29, 3)];
    gl_Position = vp_matrix * pos_world_space;
    vs_out.pos_world_space = vec3(pos_world_space);
    vs_out.tex_coords = vec3(u, v, tex_idx);
    if (u_UseAO) {
        vs_out.color = vec3(AOcurve[bitfieldExtract(data.x, 18, 2)]);
    } else {
        vs_out.color = vec3(1);
    }
}
