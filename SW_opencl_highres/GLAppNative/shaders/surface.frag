#version 150

in vec2 uv;

out vec4 color;

uniform sampler2D tex;
uniform vec2 dxy;

void main() {
    vec3 temp   = normalize(vec3(uv.x,texture(tex,uv).x*0.5,uv.y));
    vec3 temp0 	= normalize(vec3(uv.x+dxy.x,texture(tex, uv+vec2(dxy.x,0)).x*0.5,uv.y));
    vec3 temp1 	= normalize(vec3(uv.x,texture(tex, uv+vec2(0,dxy.y)).x*0.5,uv.y+dxy.y));
    vec3 norm 	= normalize(cross(temp0,temp1));
    
    vec3 light  = normalize(vec3(1.0,1.0,1.0));
    float diff 	= max(dot(norm,light), 0.5);
    
    vec4 c = texture(tex,uv);
    color = vec4(c.x*0.2,abs(c.y/c.x),abs(c.z/c.x),1.0);
}