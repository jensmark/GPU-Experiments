#version 150

in vec2 uv;

out vec4 color;

uniform vec2 dXY;
uniform sampler2D tex;

void main() {
    float rhoN  = textureOffset(tex, uv, ivec2(0,1)).x;
    float rhoS  = textureOffset(tex, uv, ivec2(0,-1)).x;
    float rhoE  = textureOffset(tex, uv, ivec2(1,0)).x;
    float rhoW  = textureOffset(tex, uv, ivec2(-1,0)).x;
    
    vec2 norm   = vec2((rhoW-rhoE)/2.0*dXY.x,
                       (rhoS-rhoN)/2.0*dXY.y);
    
    float grad  = length(norm);
    float schlieren = pow((1.0-grad/2e-3f),55.0);
    
    color = vec4(vec3(schlieren),1.0);
}