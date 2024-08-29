#version 460 core

layout(location = 0) in vec3 aPosition;

layout(std140, binding = 0) uniform Matrices
{
    mat4 vp_matrix;
    mat4 view_matrix;
    vec3 cam_pos;
};

void main() {
    gl_Position = vp_matrix * vec4(aPosition, 1.0);
}
