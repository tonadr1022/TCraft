#version 460 core

layout(location = 0) in VS_OUT {
    vec3 tex_coords;
} fs_in;

uniform sampler2DArray u_Texture;

out vec4 o_Color;

void main() {
    o_Color = texture(u_Texture, fs_in.tex_coords);
}
