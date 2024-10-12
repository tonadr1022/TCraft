#version 460 core

layout(location = 0) in VS_OUT {
    vec3 pos_world_space;
    vec3 tex_coords;
    vec3 normal;
} fs_in;

layout(binding = 0) uniform sampler2DArray u_Texture;
layout(binding = 1) uniform sampler2DArray u_shadowMap;

layout(std140, binding = 1) uniform LightSpaceMatrices {
    mat4 lightSpaceMatrices[16];
};

uniform bool u_UseTexture = true;
uniform float u_cascadePlaneDistances[16];
uniform int u_cascadeCount;
uniform vec3 u_lightDir = normalize(vec3(1, 1, 0));
uniform float u_farPlane;
uniform bool u_drawShadows;

out vec4 o_Color;

layout(std140, binding = 0) uniform Matrices {
    mat4 vp_matrix;
    mat4 view_matrix;
    vec3 cam_pos;
};

float CalculateShadow(vec3 fragPosWorldSpace) {
    vec4 fragPosViewSpace = view_matrix * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(fragPosViewSpace.z);
    int layer = -1;

    // Find the appropriate cascade layer based on the depth value
    for (int i = 0; i < u_cascadeCount; ++i) {
        if (depthValue < u_cascadePlaneDistances[i]) {
            layer = i;
            break;
        }
    }

    // If no appropriate layer is found, use the last cascade
    if (layer == -1) {
        layer = u_cascadeCount;
    }

    // Transform fragment position to light space
    vec4 fragPosLightSpace = lightSpaceMatrices[layer] * vec4(fragPosWorldSpace, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w; // Perspective divide
    projCoords = projCoords * 0.5 + 0.5; // Transform to 0-1 range

    // Depth from the light's perspective
    float currDepth = projCoords.z;

    // Return 0 if outside the light's frustum
    if (currDepth > 1.0) {
        return 0.0;
    }

    // Calculate depth bias
    vec3 normal = normalize(fs_in.normal);
    float bias = max(0.05 * (1.0 - dot(normal, u_lightDir)), 0.005);
    float biasMod = 0.5;

    // Adjust bias based on the cascade layer
    if (layer == u_cascadeCount) {
        bias *= 1.0 / (u_farPlane * biasMod);
    } else {
        bias *= 1.0 / (u_cascadePlaneDistances[layer] * biasMod);
    }

    // PCF (Percentage Closer Filtering) for soft shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(u_shadowMap, 0));
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(u_shadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, float(layer))).r;
            shadow += (currDepth - bias) > pcfDepth ? 1.0 : 0.0;
        }
    }

    shadow /= 9.0;
    return shadow;
}

void main() {
    float shadow_mult = 1.0;
    if (u_drawShadows) {
        shadow_mult = (1.0 - CalculateShadow(fs_in.pos_world_space));
    }
    if (u_UseTexture) {
        vec4 tex = texture(u_Texture, fs_in.tex_coords);
        if (tex.a < 0.5) {
            discard;
        }
        o_Color = vec4(tex.rgb * shadow_mult, 1.0);
    } else {
        o_Color = vec4(1) * shadow_mult;
    }
    // o_Color = texture(u_Texture, fs_in.tex_coords);
}
