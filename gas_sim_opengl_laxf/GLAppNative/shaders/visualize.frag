#version 150

in vec2 uv;

out vec4 color;

uniform float rx;
uniform float ry;

uniform sampler2D QTex;

void main() {
    float rhoN  = textureOffset(QTex, uv, ivec2(0,1)).x;
    float rhoS  = textureOffset(QTex, uv, ivec2(0,-1)).x;
    float rhoE  = textureOffset(QTex, uv, ivec2(1,0)).x;
    float rhoW  = textureOffset(QTex, uv, ivec2(-1,0)).x;
    
    vec2 norm   = vec2((rhoW-rhoE)/2.0*rx,
                       (rhoS-rhoN)/2.0*ry);
    float grad  = length(norm);
    float schlieren = pow((1.0-abs(grad)/1e-5f),15.0f);
    
    color = vec4(vec3(schlieren),1.0);
    //color   = abs(vec4(texture(QTex,uv)));
}