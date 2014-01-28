#version 150

in vec2 uv;

out vec4 color;

uniform sampler2D QTex;

void main() {
    float rho       = texture(QTex, uv).x;
    float schlieren = pow((1.0-abs(rho)/1.0),15);
    color = vec4(schlieren);
}