#version 460 core

in vec2 TexCoord;

out vec4 o_Color;

uniform float near_plane;
uniform float far_plane;
uniform int layer;

uniform sampler2DArray depthMap;

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // convert to NDC
    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));
}

void main() {
    float depth = texture(depthMap, vec3(TexCoord, layer)).r;
    // o_Color = vec4(vec3(LinearizeDepth(depth) / far_plane), 1.0); // linearize for perspective only
    o_Color = vec4(vec3(depth), 1.0); // linearize for perspective only
}
