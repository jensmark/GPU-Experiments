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
    
    vec2 norm   = vec2((rhoE-rhoW)/2.0*dx,
                       (rhoN-rhoS)/2.0*dy);
    float grad  = length(norm);
    float schlieren = pow((1.0-abs(grad)/2e-3),15.0);
    
    color = vec4(vec3(schlieren),1.0);
    //color   = abs(texture(QTex, uv));
}