#version 150

out vec4 color;

in vec2 uv;

uniform sampler2D QNTex;
uniform sampler2D QKTex;
uniform sampler2D FHalfTex;
uniform sampler2D GHalfTex;

uniform vec2 c;

uniform float dt;
uniform float dx;
uniform float dy;

void main(){
    vec4 FE = textureOffset(FHalfTex, uv, ivec2(1,0));
    vec4 FW = texture(FHalfTex, uv);
    vec4 GN = textureOffset(GHalfTex, uv, ivec2(0,1));
    vec4 GS = texture(GHalfTex, uv);
    
    vec4 L  = -((FE-FW)/dx+(GN-GS)/dy);
    
    vec4 Q  = texture(QNTex, uv);
    vec4 Qk = texture(QKTex, uv);
    
    color   = c.x*Q+c.y*(Qk+dt*L);
}