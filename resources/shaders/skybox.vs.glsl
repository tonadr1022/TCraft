#version 460 core

layout(location = 0) in vec3 aPos;

out vec3 TexCoords;

layout(location = 0) uniform mat4 vp_matrix;

uniform mat4 u_model;

void main() {
    TexCoords = aPos;
    vec4 pos = vp_matrix * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}
