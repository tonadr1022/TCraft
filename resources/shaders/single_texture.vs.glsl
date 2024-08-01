#version 460 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

layout(location = 0) out VS_OUT {
    vec2 tex_coords;
    flat uint material_index;
} vs_out;

// TDOO: UBO for vp matrix and other uniforms across shaders
uniform mat4 vp_matrix;

struct UniformData {
    mat4 model;
    uint material_index;
};

layout(std430, binding = 0) readonly buffer data {
    UniformData uniforms[];
};

void main() {
    UniformData uniform_data = uniforms[gl_DrawID];
    vec4 pos_world_space = uniform_data.model * vec4(aPosition, 1.0);
    gl_Position = vp_matrix * pos_world_space;
    vs_out.tex_coords = aTexCoord;
    vs_out.material_index = uniform_data.material_index;
}
