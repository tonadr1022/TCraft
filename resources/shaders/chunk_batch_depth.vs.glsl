#version 460 core

layout(location = 0) in uvec2 data;

struct UniformData {
    vec4 pos;
};

layout(std430, binding = 0) readonly buffer uniform_data_buffer {
    UniformData uniforms[];
};

uniform mat4 vp_matrix;

void main() {
    uint x = bitfieldExtract(data.x, 0, 6);
    uint y = bitfieldExtract(data.x, 6, 6);
    uint z = bitfieldExtract(data.x, 12, 6);
    UniformData uniform_data = uniforms[gl_DrawID + gl_InstanceID];
    vec4 pos_world_space = vec4(vec3(x, y, z) + uniform_data.pos.xyz, 1.0);
    gl_Position = vp_matrix * pos_world_space;
}
