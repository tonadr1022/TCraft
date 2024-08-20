#version 460 core

out vec4 o_Color;

in vec2 TexCoords;

uniform sampler2D screenTexture;

void main() {
    o_Color = texture(screenTexture, TexCoords);
}
