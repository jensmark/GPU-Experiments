#version 150

in vec2 uv;

out vec4 color;

uniform sampler2D QTex;

void main() {
    color = texture(QTex, uv);
}