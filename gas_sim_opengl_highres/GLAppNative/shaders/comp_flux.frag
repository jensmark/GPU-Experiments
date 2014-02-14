#version 150

out vec4 color0;
out vec4 color1;

in vec2 uv;

uniform sampler2D QTex;
uniform sampler2D SxTex;
uniform sampler2D SyTex;

uniform float gamma;


float P(in vec4 Q){
    return (gamma-1.0)*(Q.w-0.5*dot(Q.yz,Q.yz)/Q.x);
}

vec4 fflux(in vec4 Q){
    float u = 0.0f;
    float p = 0.0f;
    u = Q.y/Q.x;
    p = P(Q);
    return vec4(Q.y,
                (Q.y*u)+p,
                Q.z*u,
                u*(Q.w+p));
}
vec4 gflux(in vec4 Q){
    float v = 0.0f;
    float p = 0.0f;
    v = Q.z/Q.x;
    p = P(Q);
    return vec4(Q.z,
                Q.y*v,
                (Q.z*v)+p,
                v*(Q.w+p));
}

vec4 xflux(in float k, in vec4 Q, in vec4 Q1, in vec4 Sx, in vec4 Sy, in vec4 Sxp, in vec4 Syp){
    vec4 QW     = Q + Sx*0.5;
    vec4 QWp    = Q + Sx*0.5 + Sy*k;
    vec4 QWm    = Q + Sx*0.5 - Sy*k;
    
    vec4 QE     = Q1 - Sxp*0.5;
    vec4 QEp    = Q1 - Sxp*0.5 + Syp*k;
    vec4 QEm    = Q1 - Sxp*0.5 - Syp*k;
    
    float c, ap, am;
    c           = sqrt(gamma*QW.x*P(QW));
    ap          = max((QW.y+c)/QW.x,0.0);
    am          = min((QW.y-c)/QW.x,0.0);
    c           = sqrt(gamma*QE.x*P(QE));
    ap          = max((QE.y+c)/QE.x,ap);
    am          = min((QE.y-c)/QE.x,am);
    
    vec4 Fp     = ((ap*fflux(QWp) - am*fflux(QEp)) + (ap*am)*(QEp-QWp))/(ap-am);
    vec4 Fm     = ((ap*fflux(QWm) - am*fflux(QEm)) + (ap*am)*(QEm-QWm))/(ap-am);
    return mix(Fp, Fm, 0.5);
}

vec4 yflux(in float k, in vec4 Q, in vec4 Q1, in vec4 Sx, in vec4 Sy, in vec4 Sxp, in vec4 Syp){
    vec4 QS     = Q + Sy*0.5;
    vec4 QSp    = Q + Sy*0.5 + Sx*k;
    vec4 QSm    = Q + Sy*0.5 - Sx*k;
    
    vec4 QN     = Q1 - Syp*0.5;
    vec4 QNp    = Q1 - Syp*0.5 + Sxp*k;
    vec4 QNm    = Q1 - Syp*0.5 - Sxp*k;
    
    float c, ap, am;
    c           = sqrt(gamma*QS.x*P(QS));
    ap          = max((QS.z+c)/QS.x,0.0);
    am          = min((QS.z-c)/QS.x,0.0);
    c           = sqrt(gamma*QN.x*P(QN));
    ap          = max((QN.z+c)/QN.x,ap);
    am          = min((QN.z-c)/QN.x,am);
    
    vec4 Gp     = ((ap*gflux(QSp) - am*gflux(QNp)) + (ap*am)*(QNp-QSp))/(ap-am);
    vec4 Gm     = ((ap*gflux(QSm) - am*gflux(QNm)) + (ap*am)*(QNm-QSm))/(ap-am);
    return mix(Gp, Gm, 0.5);

}

void main(){
    const float k = 0.2886751346;
    vec4 Q      = texture(QTex, uv);
    vec4 Sx     = texture(SxTex, uv);
    vec4 Sy     = texture(SyTex, uv);
    
    vec4 Q1     = textureOffset(QTex, uv, ivec2(1,0));
    vec4 Sxp    = textureOffset(SxTex, uv, ivec2(1,0));
    vec4 Syp    = textureOffset(SyTex, uv, ivec2(1,0));
    
    color0 = xflux(k, Q, Q1, Sx, Sy, Sxp, Syp);
    
    Q1     = textureOffset(QTex, uv, ivec2(0,1));
    Sxp    = textureOffset(SxTex, uv, ivec2(0,1));
    Syp    = textureOffset(SyTex, uv, ivec2(0,1));
    
    color1 = yflux(k, Q, Q1, Sx, Sy, Sxp, Syp);
}