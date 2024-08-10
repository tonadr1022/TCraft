#version 460

layout(location = 0) in VS_OUT {
    vec3 color;
} fs_in;

out vec4 o_Color;

void main() {
    o_Color = vec4(fs_in.color, 1.0);
}
