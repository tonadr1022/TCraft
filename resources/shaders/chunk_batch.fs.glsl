#version 460 core

layout(location = 0) in VS_OUT {
    vec3 pos_world_space;
    vec3 tex_coords;
    vec3 color;
} fs_in;

uniform sampler2DArray u_Texture;
uniform bool u_UseTexture = true;

out vec4 o_Color;

void main() {
    if (u_UseTexture) {
        vec4 tex = texture(u_Texture, fs_in.tex_coords);
        o_Color = vec4(tex.rgb * fs_in.color, tex.a);
    } else {
        o_Color = vec4(fs_in.color, 1);
    }
}
