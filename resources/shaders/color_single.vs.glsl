#version 460 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

uniform mat4 model;

// std140 explicitly states the memory layout.
// https://registry.khronos.org/OpenGL/extensions/ARB/ARB_uniform_buffer_object.txt
layout(std140, binding = 0) uniform Matrices
{
    mat4 vp_matrix;
    mat4 view_matrix;
    vec3 cam_pos;
};

void main() {
    gl_Position = vp_matrix * model * vec4(aPosition, 1.0);
}
