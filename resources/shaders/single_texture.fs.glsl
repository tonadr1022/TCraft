#version 460 core
#extension GL_ARB_bindless_texture : enable

layout(location = 0) in VS_OUT {
    vec2 tex_coords;
    flat uint material_index;
} fs_in;

out vec4 o_Color;

struct Material {
    uvec2 tex_handle;
};

layout(std430, binding = 1) readonly buffer Materials {
    Material materials[];
};

void main() {
    Material material = materials[fs_in.material_index];
    const bool hasTex = (material.tex_handle.x != 0 || material.tex_handle.y != 0);
    if (hasTex) {
        o_Color = texture(sampler2D(material.tex_handle), fs_in.tex_coords);
        if (o_Color.a < 0.5) {
            discard;
        }
    } else {
        o_Color = vec4(0);
    }
}
