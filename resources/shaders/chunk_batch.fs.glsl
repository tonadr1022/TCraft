#version 460 core

layout(location = 0) in VS_OUT {
    vec3 pos_world_space;
    vec3 tex_coords;
    vec3 color;
    vec3 normal;
} fs_in;

uniform sampler2DArray u_Texture;
uniform bool u_UseTexture = true;
uniform sampler2DArray u_shadowMap;

layout(std140, binding = 0) uniform Matrices
{
    mat4 vp_matrix;
    mat4 view_matrix;
    vec3 cam_pos;
};

layout(std140, binding = 1) uniform LightSpaceMatrices {
    mat4 lightSpaceMatrices[16];
};

uniform float u_cascadePlaneDistances[16];
uniform int u_cascadeCount;
uniform vec3 u_lightDir = normalize(vec3(1, 1, 0));
uniform float u_farPlane;
uniform bool u_drawShadows;

out vec4 o_Color;

float CalculateShadow(vec3 fragPosWorldSpace) {
    vec4 fragPosViewSpace = view_matrix * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(fragPosViewSpace.z);
    int layer = -1;
    for (int i = 0; i < u_cascadeCount; ++i) {
        if (depthValue < u_cascadePlaneDistances[i]) {
            layer = i;
            break;
        }
    }
    if (layer == -1) {
        layer = u_cascadeCount;
    }

    vec4 fragPosLightSpace = lightSpaceMatrices[layer] * vec4(fragPosWorldSpace, 1.0);
    // perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to 0,1 range
    projCoords = projCoords * 0.5 + 0.5;
    // depth from light's perspective
    float currDepth = projCoords.z;
    // keep shadow at 0 when outside far plane of light's frustum
    if (currDepth > 1.0) {
        return 0.0;
    }

    vec3 normal = normalize(fs_in.normal);
    float bias = max(0.05 * (1.0 - dot(normal, u_lightDir)), 0.005);
    const float biasMod = 0.5;
    if (layer == u_cascadeCount) {
        bias *= 1 / (u_farPlane * biasMod);
    } else {
        bias *= 1 / (u_cascadePlaneDistances[layer] * biasMod);
    }

    // PCF - soft shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(u_shadowMap, 0));
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(u_shadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r;
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
        if (tex.a < 0.5) discard;
        o_Color = vec4(tex.rgb * fs_in.color, tex.a) * shadow_mult;
    } else {
        o_Color = vec4(fs_in.color, 1) * shadow_mult;
    }
}
