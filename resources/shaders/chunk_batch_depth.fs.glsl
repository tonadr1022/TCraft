#version 460 core

layout(binding = 0) uniform sampler2DArray u_Texture;

layout(location = 0) in VS_OUT {
    vec3 tex_coords;
} fs_in;

void main() {
    vec4 tex = texture(u_Texture, fs_in.tex_coords);
    if (tex.a < 0.5) discard;
}
