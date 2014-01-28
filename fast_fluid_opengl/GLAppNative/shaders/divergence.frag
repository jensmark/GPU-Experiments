#version 150

out vec4 color;

in vec2 uv;

uniform float hr;

uniform sampler2D WTex;

void main(){
    vec4 wL = textureOffset(WTex, uv, ivec2(-1,0));
    vec4 wR = textureOffset(WTex, uv, ivec2(1,0))
    vec4 wB = textureOffset(WTex, uv, ivec2(0,-1));
    vec4 wT = textureOffset(WTex, uv, ivec2(0,1));

    color = hr * ((wR.x - wL.x) + (wT.y - wB.y));
}