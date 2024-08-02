#version 460 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aTexCoord;

layout(location = 0) out VS_OUT {
    vec3 pos_world_space;
    vec3 tex_coords;
} vs_out;

uniform mat4 vp_matrix;

struct UniformData {
    mat4 model;
};

layout(std430, binding = 0) readonly buffer data {
    UniformData uniforms[];
};

void main() {
    UniformData uniform_data = uniforms[gl_DrawID + gl_InstanceID];
    vec4 pos_world_space = uniform_data.model * vec4(aPosition, 1.0);
    gl_Position = vp_matrix * pos_world_space;
    vs_out.pos_world_space = vec3(pos_world_space);
    vs_out.tex_coords = aTexCoord;
}
