#version 460 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

/* layout(std140, binding = 0) uniform Matrices {
    mat4 vp_matrix;
    vec3 cam_pos;
}
    */

layout(location = 0) out VS_OUT {
    vec3 pos_world_space;
    vec3 tex_coords;
} vs_out;

uniform mat4 model_matrix;
uniform mat4 vp_matrix;

void main() {
    vec4 pos_world_space = model_matrix * vec4(aPosition, 1.0);
    gl_Position = vp_matrix * pos_world_space;
    vs_out.pos_world_space = vec3(pos_world_space);
    vs_out.tex_coords = vec3(aTexCoord, 0);
}
