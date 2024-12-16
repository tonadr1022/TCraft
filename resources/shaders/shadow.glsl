layout(std140, binding = 1) uniform LightSpaceMatrices {
    mat4 lightSpaceMatrices[16];
};

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
    float bias = max(0.01 * (1.0 - dot(normal, u_lightDir)), 0.005);
    const float biasMod = 0.5;
    if (layer == u_cascadeCount - 1) {
        bias *= 1.0 / (u_farPlane * biasMod);
    } else {
        bias *= 1.0 / (u_cascadePlaneDistances[layer] * biasMod);
    }

    // PCF - soft shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(u_shadowMap, 0));
    #define RANGE 2
    #define COUNT 25
    for (int x = -RANGE; x <= RANGE; ++x) {
        for (int y = -RANGE; y <= RANGE; ++y) {
            float pcfDepth = texture(u_shadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r;
            shadow += (currDepth - bias) > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= COUNT;
    return shadow;
}

// Array of colors for each cascade layer
vec3 cascadeColors[5] = vec3[](
        vec3(1.0, 0.0, 0.0),
        vec3(0.0, 1.0, 0.0),
        vec3(0.0, 0.0, 1.0),
        vec3(1.0, 1.0, 0.0),
        vec3(0.0, 1.0, 1.0)
    );

vec3 CascadeDebugColor(vec3 fragPosWorldSpace) {
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
        return vec3(1.0, 1.0, 1.0);
    }
    return cascadeColors[layer];
}
