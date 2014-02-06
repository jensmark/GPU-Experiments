#version 150

in vec2 uv;

out vec4 color;

uniform sampler2D QTex;
uniform float rx;
uniform float ry;
uniform float gamma;

float P(in vec2 uv, in float E){
    return (gamma-1.0)*(E-0.5*dot(uv,uv));
}

vec4 fflux(in vec4 Q){
    float u = 0.0;
    float v = 0.0;
    
    //if(Q.x != 0.0){
        u = Q.y/Q.x;
        v = Q.z/Q.x;
    //}
    
    float p = P(vec2(u,v),Q.w);
    return vec4(Q.y,
                (Q.y*u)+p,
                Q.z*u,
                u*(Q.w+p));
}
vec4 gflux(in vec4 Q){
    float u = 0.0;
    float v = 0.0;
    
    //if(Q.x != 0.0){
        u = Q.y/Q.x;
        v = Q.z/Q.x;
    //}
    
    float p = P(vec2(u,v),Q.w);
    return vec4(Q.z,
                Q.y*v,
                (Q.z*v)+p,
                v*(Q.w+p));
}

void main() {
    vec4 QE = textureOffset(QTex, uv, ivec2(1,0));
    vec4 QW = textureOffset(QTex, uv, ivec2(-1,0));
    vec4 QN = textureOffset(QTex, uv, ivec2(0,1));
    vec4 QS = textureOffset(QTex, uv, ivec2(0,-1));
    
    color = 0.25*(QE + QW + QN + QS) - 0.5*rx*(fflux(QE) - fflux(QW)) - 0.5*ry*(gflux(QN) - gflux(QS));
}