#version 150

in vec2 uv;

out vec4 color;

uniform float rx;
uniform float ry;

uniform sampler2D QTex;

void main() {
    float QE = textureOffset(QTex, uv, ivec2(1,0)).x;
    float QW = textureOffset(QTex, uv, ivec2(-1,0)).x;
    float QN = textureOffset(QTex, uv, ivec2(0,1)).x;
    float QS = textureOffset(QTex, uv, ivec2(0,-1)).x;
    
    float rho = ((QN-QS)/ry)+((QE-QW)/rx);
    float schlieren = pow((1.0-abs(rho)/1.0),15);
    color = vec4(schlieren);
}