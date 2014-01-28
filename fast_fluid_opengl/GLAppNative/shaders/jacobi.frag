#version 150

out vec4 color;

in vec2 uv;

uniform float alpha;
uniform float beta;

uniform sampler2D XTex;
uniform sampler2D BTex;

void main(){
    vec4 xL = textureOffset(XTex, uv, ivec2(-1,0));
    vec4 xR = textureOffset(XTex, uv, ivec2(1,0))
    vec4 xB = textureOffset(XTex, uv, ivec2(0,-1));
    vec4 xT = textureOffset(XTex, uv, ivec2(0,1));
    
    vec4 bC = texture(BTex, uv);
    
    color = (xL + xR + xB + xT + alpha * bC) * beta;
}