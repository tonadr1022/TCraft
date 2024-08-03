#version 460 core

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

uniform mat4 projection;
uniform float line_width;

void main() {
    vec4 p0 = gl_in[0].gl_Position;
    vec4 p1 = gl_in[1].gl_Position;

    vec2 line_direction = normalize(p1.xy - p0.xy);
    vec2 normal = vec2(-line_direction.y, line_direction.x);

    vec4 offset = vec4(normal * line_width / 2.0, 0.0, 0.0);

    gl_Position = projection * (p0 + offset);
    EmitVertex();

    gl_Position = projection * (p0 - offset);
    EmitVertex();

    gl_Position = projection * (p1 + offset);
    EmitVertex();

    gl_Position = projection * (p1 - offset);
    EmitVertex();

    EndPrimitive();
}
