#version 150

out vec4 color;

in vec2 uv;

uniform float dt;
uniform float r

uniform sampler2D UTex;
uniform sampler2D XTex;

vec4 textureBilinear(in sampler2D tex, in vec2 uv) {
    vec4 tl = texture(tex, uv);
    vec4 tr = textureOffset(tex, uv, ivec2(1, 0));
    vec4 bl = textureOffset(tex, uv, ivec2(0, 1));
    vec4 br = textureOffset(tex, uv, ivec2(1, 1));
    vec2 f  = fract(uv.xy * vec2(textureSize(tex)));
    vec4 tA = mix(tl, tr, f.x);
    vec4 tB = mix(bl, br, f.x);
    return mix(tA, tB, f.y);
}

void main(){
    vec2 pos    = uv-dt*r*texture(UTex,uv);
    color       = textureBilinear(XTex,pos);
}