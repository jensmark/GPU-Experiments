#version 150

in vec2 uv;

out vec4 color;

uniform sampler2D UTex;

void main() {
    color = texture(UTex, uv); // TODO: implement way to visualize velocity field
}