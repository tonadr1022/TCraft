#version 460

uniform vec3 color;

out vec4 o_Color;

void main() {
    o_Color = vec4(color, 1.0);
}
