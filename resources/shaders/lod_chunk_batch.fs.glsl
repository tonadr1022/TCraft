#version 460 core

layout(location = 0) in VS_OUT {
    vec3 pos_world_space;
    vec3 color;
} fs_in;

out vec4 o_Color;

void main() {
    o_Color = vec4(fs_in.color.rgb, 1.0);
}
