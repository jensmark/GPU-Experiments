#version 150

in vec2 uv;

out vec4 color;

uniform sampler2D QTex;
/*uniform float gamma;

float E(in float rho, in vec2 uv){
    float p = 1.0;//(gamma-1.0)*(E+0.5*dot(uv,uv));
    return 0.5*rho*dot(uv,uv)+p/(gamma-1.0);
}*/

void main() {
    /*color.xyz = texture(QTex, uv).xyz;
    float u = color.y/color.x;
    if (isnan(u)) {
        u = 0.0;
    }
    float v = color.z/color.x;
    if (isnan(v)) {
        v = 0.0;
    }
    color.w = E(color.x,vec2(u,v));*/
    color = texture(QTex, uv);
}