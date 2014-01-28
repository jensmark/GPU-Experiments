#version 150

out vec4 color0;
out vec4 color1;

in vec2 uv;

uniform sampler2D QTex;

uniform float dx;
uniform float dy;

vec4 minmod(in vec4 a, in vec4 b){
    vec4 res = min(abs(a),abs(b));
    return res*(sign(a)+sign(b))*0.5;
}

void main(){
    vec4 Q  = texture(QTex, uv);
    
    vec4 QE = textureOffset(QTex, uv, ivec2(1,0));
    vec4 QW = textureOffset(QTex, uv, ivec2(-1,0));
    color0  = minmod(Q-QW,QE-Q);
    
    vec4 QN = textureOffset(QTex, uv, ivec2(0,1));
    vec4 QS = textureOffset(QTex, uv, ivec2(0,-1));
    color1  = minmod(Q-QS,QN-Q);
}