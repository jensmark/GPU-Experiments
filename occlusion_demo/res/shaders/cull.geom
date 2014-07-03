#version 400 core

layout(points) in;
layout(points, max_vertices = 1) out;

flat in int visible[1];
flat out int outVisible;

void main() {
    gl_Position = gl_in[0].gl_Position;
    outVisible = visible[0];
    EmitVertex();
}