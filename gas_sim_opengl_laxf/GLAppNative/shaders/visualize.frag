#version 150

in vec2 uv;

out vec4 color;

uniform float rx;
uniform float ry;

uniform sampler2D QTex;

void main() {
    float rho   = texture(QTex, uv).x;
    float grad  = ((textureOffset(QTex,uv,ivec2(1,0)).x-rho)/rx)+
                ((textureOffset(QTex,uv,ivec2(0,1)).x-rho)/ry);
    float schlieren = pow((1.0-abs(grad)/1.0),15);
    color = vec4(schlieren);
}