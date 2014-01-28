#version 150

out vec4 color;

in vec2 uv;

uniform float hr;

uniform sampler2D WTex;
uniform sampler2D PTex;

void main(){
    vec4 pL = textureOffset(PTex, uv, ivec2(-1,0));
    vec4 pR = textureOffset(PTex, uv, ivec2(1,0))
    vec4 pB = textureOffset(PTex, uv, ivec2(0,-1));
    vec4 pT = textureOffset(PTex, uv, ivec2(0,1));
    
    color = texture(WTex, uv);
    color.xy -= hr * vec2(pR - pL, pT - pB);
}