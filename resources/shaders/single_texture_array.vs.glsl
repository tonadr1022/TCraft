#version 460 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

layout(location = 0) out VS_OUT {
    vec3 tex_coords;
    flat uint material_index;
} vs_out;

struct UniformData {
    mat4 model;
    uint material_index;
    uint layer_index;
};

layout(std430, binding = 0) readonly buffer data {
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

void main() {
    UniformData uniform_data = uniforms[gl_DrawID + gl_InstanceID];
    vec4 pos_world_space = uniform_data.model * vec4(aPosition, 1.0);
    gl_Position = vp_matrix * pos_world_space;
    vs_out.tex_coords = vec3(aTexCoord, uniform_data.layer_index);
    vs_out.material_index = uniform_data.material_index;
}
