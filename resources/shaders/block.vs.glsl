#version 460 core

layout(location = 0) in uvec2 data;

layout(location = 0) out VS_OUT {
    vec3 tex_coords;
} vs_out;

uniform mat4 vp_matrix;

void main() {
    uint x = bitfieldExtract(data.x, 0, 6);
    uint y = bitfieldExtract(data.x, 6, 6);
    uint z = bitfieldExtract(data.x, 12, 6);
    uint u = bitfieldExtract(data.x, 20, 6);
    uint v = bitfieldExtract(data.x, 26, 6);
    uint tex_idx = bitfieldExtract(data.y, 0, 32);
    gl_Position = vp_matrix * vec4(x, y, z, 1.0);
    vs_out.tex_coords = vec3(u, v, tex_idx);
}
