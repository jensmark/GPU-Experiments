#version 150

in vec2 uv;

out vec4 color;

uniform sampler2D QTex;
uniform float rx;
uniform float ry;
uniform float gamma;
uniform float p;

float E(in float rho, in float u, in float v){
    return 0.5*rho*(u*u+v*v)+p/(gamma-1.0);
}

vec4 fflux(in vec4 Q){
    float u = Q.y/Q.x;
    if (isnan(u)) {
        u = 0.0;
    }
    float v = Q.z/Q.x;
    if (isnan(v)) {
        v = 0.0;
    }
    return vec4(Q.y,
                (Q.y*u)+p,
                Q.z*u,
                u*(E(Q.x,u,v)+p));
}
vec4 gflux(in vec4 Q){
    float u = Q.y/Q.x;
    if (isnan(u)) {
        u = 0.0;
    }
    float v = Q.z/Q.x;
    if (isnan(v)) {
        v = 0.0;
    }
    return vec4(Q.z,
                Q.y*v,
                (Q.z*v)+p,
                v*(E(Q.x,u,v)+p));
}

void main() {
    vec4 QE = textureOffset(QTex, uv, ivec2(1,0));
    vec4 QW = textureOffset(QTex, uv, ivec2(-1,0));
    vec4 QN = textureOffset(QTex, uv, ivec2(0,1));
    vec4 QS = textureOffset(QTex, uv, ivec2(0,-1));
    
    color = 0.25*(QE + QW + QN + QS) - 0.5*rx*(fflux(QE) - fflux(QW)) - 0.5*ry*(gflux(QN) - gflux(QS));
}