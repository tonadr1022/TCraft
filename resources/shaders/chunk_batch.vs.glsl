#version 460 core

layout(location = 0) in uvec2 data;

layout(location = 0) out VS_OUT {
    vec3 pos_world_space;
    vec3 tex_coords;
} vs_out;

uniform mat4 vp_matrix;

struct UniformData {
    mat4 model;
};

layout(std430, binding = 0) readonly buffer uniform_data_buffer {
    UniformData uniforms[];
};

void main() {
    float x = float(bitfieldExtract(data.x, 0, 6));
    float y = float(bitfieldExtract(data.x, 6, 6));
    float z = float(bitfieldExtract(data.x, 12, 6));
    uint u = bitfieldExtract(data.x, 18, 1);
    uint v = bitfieldExtract(data.x, 19, 1);
    uint tex_idx = bitfieldExtract(data.x, 20, 12);
    UniformData uniform_data = uniforms[gl_DrawID + gl_InstanceID];
    vec3 pos = vec3(x, y, z);
    vec4 pos_world_space = uniform_data.model * vec4(pos, 1.0);
    gl_Position = vp_matrix * pos_world_space;
    vs_out.pos_world_space = vec3(pos_world_space);
    vs_out.tex_coords = vec3(u, v, tex_idx);
}
