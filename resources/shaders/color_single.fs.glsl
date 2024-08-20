#version 460

out vec4 o_Color;

uniform vec4 color;

void main() {
    o_Color = color;
    // o_Color = vec4(0, 0, 0, 1);
}
