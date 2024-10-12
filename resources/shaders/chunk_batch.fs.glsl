#version 460 core

layout(location = 0) in VS_OUT {
    vec3 pos_world_space;
    vec3 tex_coords;
    vec3 color;
    vec3 normal;
} fs_in;

layout(binding = 0) uniform sampler2DArray u_Texture;
layout(binding = 1) uniform sampler2DArray u_shadowMap;

layout(std140, binding = 0) uniform Matrices {
    mat4 vp_matrix;
    mat4 view_matrix;
    vec3 cam_pos;
};

uniform bool u_cascadeDebugColors = false;
uniform bool u_UseTexture = true;
uniform float u_cascadePlaneDistances[16];
uniform int u_cascadeCount;
uniform vec3 u_lightDir = normalize(vec3(1, 1, 0));
uniform float u_farPlane;
uniform bool u_drawShadows;
uniform vec3 u_ambient_color = vec3(0.7);

out vec4 o_Color;

#include "shadow.glsl"

void main() {
    if (u_cascadeDebugColors) {
        o_Color = vec4(CascadeDebugColor(fs_in.pos_world_space), 1.0);
        return;
    }
    float diff = max(dot(u_lightDir, normalize(fs_in.normal)), 0.0);

    float shadow_mult = 1.0;
    if (u_drawShadows) {
        shadow_mult = (1.0 - CalculateShadow(fs_in.pos_world_space));
        if (diff == 0.0 || u_lightDir.y < 0) shadow_mult = 0.0;
    }
    if (u_UseTexture) {
        vec4 tex = texture(u_Texture, fs_in.tex_coords);
        if (tex.a < 0.5) discard;
        vec3 color = tex.rgb * fs_in.color;
        o_Color = vec4(color * shadow_mult + color * u_ambient_color * (1 - shadow_mult), tex.a);
    } else {
        o_Color = vec4(fs_in.color, 1) * shadow_mult;
    }
}
