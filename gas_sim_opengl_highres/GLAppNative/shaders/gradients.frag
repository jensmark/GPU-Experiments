#version 150

in vec2 uv;

out vec4 color;

uniform float dx;
uniform float dy;
uniform sampler2D QTex;

void main() {
    float rhoN  = textureOffset(QTex, uv, ivec2(0,1)).x;
    float rhoS  = textureOffset(QTex, uv, ivec2(0,-1)).x;
    float rhoE  = textureOffset(QTex, uv, ivec2(1,0)).x;
    float rhoW  = textureOffset(QTex, uv, ivec2(-1,0)).x;
    
    vec2 norm   = vec2((rhoW-rhoE)/2.0*dx,
                       (rhoS-rhoN)/2.0*dy);
    color = vec4(length(norm));
}