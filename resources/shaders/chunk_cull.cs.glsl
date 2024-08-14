#version 460 core

#define FullyVisible    2
#define PartiallyVisible 1
#define Invisible       0
struct DrawElementsCommand
{
    uint count; // num indices in draw call
    uint instance_count; // num instances in draw call
    uint first_index; // offset in index buffer: sizeof(element)*firstIndex from start of buffer
    uint base_vertex; // offset in vertex buffer: sizeof(vertex)*baseVertex from start of buffer
    uint base_instance; // first instance to draw (position in instanced buffer)
};

struct Frustum {
    float data_[6][4];
};

// padding for GPU
struct AABB {
    vec4 min;
    vec4 max;
};

struct DrawInfo {
    uvec2 handle;
    uint vertex_offset;
    uint vertex_size;
    AABB aabb;
    uint first_index; // ebo
    uint count; // ebo
};

struct UniformData {
    vec4 pos;
};

layout(std430, binding = 0) readonly restrict buffer ssbo_0 {
    DrawInfo in_draw_info[];
};

layout(std430, binding = 1) writeonly restrict buffer dib_3 {
    DrawElementsCommand out_draw_cmds[];
};

layout(std430, binding = 2) writeonly restrict buffer dib_4 {
    UniformData out_uniforms[];
};

// This can be used as a parameter buffer in multiDrawElementsIndirectCount()
// https://registry.khronos.org/OpenGL/extensions/ARB/ARB_indirect_parameters.txt
layout(std430, binding = 3) coherent restrict buffer parameter_buffer_4 {
    uint next_idx;
};

uniform vec4 plane0;
uniform vec4 plane1;
uniform vec4 plane2;
uniform vec4 plane3;
uniform vec4 plane4;
uniform vec4 plane5;

uniform float u_min_cull_dist = 1;
uniform float u_max_cull_dist = 99999999;
uniform vec3 u_view_pos;
uniform bool u_cull_frustum = true;
uniform uint u_vertex_size = 8;

bool CullDistance(float dist, float minDist, float maxDist);
float CalcDistance(AABB box, vec3 pos);
int CullFrustum(AABB box);

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main() {
    if (gl_GlobalInvocationID.x >= in_draw_info.length()) return;
    DrawInfo draw_info = in_draw_info[gl_GlobalInvocationID.x];
    // Draw if there are vertices to draw and it's in the view frustum
    bool should_draw = false;
    uint frustum_val = 0;
    if (draw_info.handle != uvec2(0)) {
        frustum_val = CullFrustum(draw_info.aabb);
        if (
            CullDistance(CalcDistance(draw_info.aabb, u_view_pos), u_min_cull_dist, u_max_cull_dist) &&
                (!u_cull_frustum || frustum_val != 0)
        ) {
            should_draw = true;
        }
    }
    if (should_draw) {
        DrawElementsCommand cmd;
        cmd.count = draw_info.count; // indices count
        cmd.instance_count = 1; // 1 since not instancing, would be zero if not drawn, but 0 instance count commands are skipped
        cmd.first_index = draw_info.first_index; // offset into the element buffer object for indices
        cmd.base_vertex = draw_info.vertex_offset / u_vertex_size; // index of first vertex
        // atomic index shared across all threads
        uint insert = atomicAdd(next_idx, 1);
        // TODO: figure out why this doesn't seem to matter?
        cmd.base_instance = 0;
        UniformData d;
        // Adding frustum val for debug
        d.pos = vec4(draw_info.aabb.min.x, draw_info.aabb.min.y, draw_info.aabb.min.z, frustum_val);
        out_uniforms[insert] = d;
        out_draw_cmds[insert] = cmd;
    }
}

bool CullDistance(float dist, float minDist, float maxDist) {
    return dist >= minDist && dist <= maxDist;
}

float CalcDistance(AABB box, vec3 pos) {
    return distance(pos, ((box.min.xyz + box.max.xyz) / 2));
}

int GetVisibility(vec4 clip, AABB box) {
    // get the dimensions
    float x0 = box.min.x * clip.x;
    float x1 = box.max.x * clip.x;
    float y0 = box.min.y * clip.y;
    float y1 = box.max.y * clip.y;
    float z0 = box.min.z * clip.z + clip.w;
    float z1 = box.max.z * clip.z + clip.w;
    // Get the 8 points of the aabb in clip space
    float p1 = x0 + y0 + z0;
    float p2 = x1 + y0 + z0;
    float p3 = x1 + y1 + z0;
    float p4 = x0 + y1 + z0;
    float p5 = x0 + y0 + z1;
    float p6 = x1 + y0 + z1;
    float p7 = x1 + y1 + z1;
    float p8 = x0 + y1 + z1;

    // If all the points are outside the plane, it's invisible
    if (p1 <= 0 && p2 <= 0 && p3 <= 0 && p4 <= 0 && p5 <= 0 && p6 <= 0 && p7 <= 0 && p8 <= 0) {
        return Invisible;
    }
    // If all the points are in the plane, it's fully visible
    if (p1 > 0 && p2 > 0 && p3 > 0 && p4 > 0 && p5 > 0 && p6 > 0 && p7 > 0 && p8 > 0) {
        return FullyVisible;
    }
    // partial vis
    return PartiallyVisible;
}

int CullFrustum(AABB box) {
    int v0 = GetVisibility(plane0, box);
    if (v0 == 0) {
        return Invisible;
    }
    int v1 = GetVisibility(plane1, box);
    if (v1 == 0) {
        return Invisible;
    }
    int v2 = GetVisibility(plane2, box);
    if (v2 == 0) {
        return Invisible;
    }
    int v3 = GetVisibility(plane3, box);
    if (v3 == 0) {
        return Invisible;
    }
    int v4 = GetVisibility(plane4, box);
    if (v4 == 0) {
        return Invisible;
    }
    int v5 = GetVisibility(plane5, box);
    if (v5 == 0) {
        return Invisible;
    }

    if (v0 == 1 && v1 == 1 && v2 == 1 && v3 == 1 && v4 == 1 && v5 == 1) {
        return FullyVisible;
    }
    return PartiallyVisible;
}
