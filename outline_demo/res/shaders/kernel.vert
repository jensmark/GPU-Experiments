#version 150

in vec2 position;

out vec2 UV;

void main() {
    UV = vec2(0.5)+(position)*0.5;
    gl_Position = vec4(position, 0.0, 1.0);
}