#version 460 core

layout(location = 0) in uvec2 data;

layout(location = 0) out VS_OUT {
    vec3 pos_world_space;
    vec3 tex_coords;
    vec3 color;
} vs_out;

uniform mat4 vp_matrix;

struct UniformData {
    mat4 model;
};

layout(std430, binding = 0) readonly buffer uniform_data_buffer {
    UniformData uniforms[];
};

const float AOcurve[4] = float[4](0.55, 0.75, 0.87, 1.0);

void main() {
    uint x = bitfieldExtract(data.x, 0, 6);
    uint y = bitfieldExtract(data.x, 6, 6);
    uint z = bitfieldExtract(data.x, 12, 6);
    uint u = bitfieldExtract(data.x, 20, 6);
    uint v = bitfieldExtract(data.x, 26, 6);
    uint tex_idx = bitfieldExtract(data.y, 0, 32);
    UniformData uniform_data = uniforms[gl_DrawID + gl_InstanceID];
    vec4 pos_world_space = uniform_data.model * vec4(x, y, z, 1.0);
    gl_Position = vp_matrix * pos_world_space;
    vs_out.pos_world_space = vec3(pos_world_space);
    vs_out.tex_coords = vec3(u, v, tex_idx);
    vs_out.color = vec3(AOcurve[bitfieldExtract(data.x, 18, 2)]);
}
