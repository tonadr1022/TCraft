#version 460 core

layout(location = 0) in vec3 v_Position;

/* layout(std140, binding = 0) uniform Matrices {
    mat4 vp_matrix;
    vec3 cam_pos;
}
    */
uniform mat4 model_matrix;
uniform mat4 vp_matrix;

void main() {
    gl_Position = vp_matrix * model_matrix * vec4(v_Position, 1.0);
}
