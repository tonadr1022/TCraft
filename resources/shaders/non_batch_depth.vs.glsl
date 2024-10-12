#version 410 core

layout(location = 0) in vec3 aPos;

uniform mat4 vp_matrix;
uniform mat4 model;

void main() {
    gl_Position = vp_matrix * model * vec4(aPos, 1.0);
}
