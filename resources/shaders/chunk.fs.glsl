#version 460 core

layout(location = 0) in VS_OUT {
    vec3 pos_world_space;
    vec3 tex_coords;
} fs_in;

out vec4 o_Color;

uniform sampler2DArray u_Texture;

void main() {
    o_Color = texture(u_Texture, fs_in.tex_coords);
}
