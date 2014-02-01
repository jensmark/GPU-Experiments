#version 150

in vec2 uv;

out vec4 color;

uniform float rx;
uniform float ry;

uniform sampler2D QTex;

void main() {
    float rho   = texture(QTex, uv).x;
    
    vec2 norm   = vec2((rho-textureOffset(QTex,uv,ivec2(1,0)).x)/rx,
                                 (rho-textureOffset(QTex,uv,ivec2(0,1)).x)/ry);
    float grad  = length(norm);
    float schlieren = pow((1.0-abs(grad)/150.0),15);
    
    color = vec4(schlieren);
    //color = vec4(abs(norm), 0.0,1.0);
    //color = vec4(rho);
    //color = vec4(texture(QTex, uv).yzw, 1.0);
    //color = texture(QTex, uv);
}