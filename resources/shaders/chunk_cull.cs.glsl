#version 460 core

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
    uint _pad2;
    uint first_index; // ebo
    uint count; // ebo
    uint _pad3;
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

layout(std430, binding = 3) coherent restrict buffer parameter_buffer_4 {
    uint next_idx;
};

layout(location = 0) uniform Frustum u_viewfrustum;

bool CullDistance(float dist, float minDist, float maxDist);
int CullFrustum(in AABB box, in Frustum frustum);

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main() {
    if (gl_GlobalInvocationID.x >= in_draw_info.length()) return;
    DrawInfo draw_info = in_draw_info[gl_GlobalInvocationID.x];
    // Draw if there are vertices to draw and it's in the view frustum
    bool should_draw = draw_info.handle != uvec2(0) && CullFrustum(draw_info.aabb, u_viewfrustum) != 0;
    if (draw_info.handle != uvec2(0)) {
        DrawElementsCommand cmd;
        cmd.count = draw_info.count;
        cmd.instance_count = 1;
        cmd.first_index = draw_info.first_index;
        cmd.base_vertex = draw_info.vertex_offset / 8;
        uint insert = atomicAdd(next_idx, 1);
        // size of vertex is 8 bytes
        cmd.base_instance = 0;
        // cmd.base_instance = draw_info.vertex_offset / 8;
        UniformData d;
        d.pos = vec4(draw_info.aabb.min.x, draw_info.aabb.min.y, draw_info.aabb.min.z, 0);
        out_uniforms[insert] = d;
        out_draw_cmds[insert] = cmd;
    }
}

vec4 GetPlane(int plane, in Frustum frustum) {
    return vec4(frustum.data_[plane][0], frustum.data_[plane][1],
        frustum.data_[plane][2], frustum.data_[plane][3]);
}

int GetVisibility(in vec4 clip, in AABB box) {
    float x0 = box.min.x * clip.x;
    float x1 = box.max.x * clip.x;
    float y0 = box.min.y * clip.y;
    float y1 = box.max.y * clip.y;
    float z0 = box.min.z * clip.z + clip.w;
    float z1 = box.max.z * clip.z + clip.w;
    float p1 = x0 + y0 + z0;
    float p2 = x1 + y0 + z0;
    float p3 = x1 + y1 + z0;
    float p4 = x0 + y1 + z0;
    float p5 = x0 + y0 + z1;
    float p6 = x1 + y0 + z1;
    float p7 = x1 + y1 + z1;
    float p8 = x0 + y1 + z1;

    if (p1 <= 0 && p2 <= 0 && p3 <= 0 && p4 <= 0 && p5 <= 0 && p6 <= 0 && p7 <= 0 && p8 <= 0) {
        return 0;
    }
    if (p1 > 0 && p2 > 0 && p3 > 0 && p4 > 0 && p5 > 0 && p6 > 0 && p7 > 0 && p8 > 0) {
        return 1;
    }
    // partial vis
    return 1;
}

int CullFrustum(in AABB box, in Frustum frustum) {
    int v0 = GetVisibility(GetPlane(0, frustum), box);
    if (v0 == 0) {
        return 0;
    }
    int v1 = GetVisibility(GetPlane(1, frustum), box);
    if (v1 == 0) {
        return 0;
    }
    int v2 = GetVisibility(GetPlane(2, frustum), box);
    if (v2 == 0) {
        return 0;
    }
    int v3 = GetVisibility(GetPlane(3, frustum), box);
    if (v3 == 0) {
        return 0;
    }
    int v4 = GetVisibility(GetPlane(4, frustum), box);
    if (v4 == 0) {
        return 0;
    }
    if (v0 == 1 && v1 == 1 && v2 == 1 && v3 == 1 && v4 == 1) {
        return 1;
    }
    // partial vis
    return 1;
}
