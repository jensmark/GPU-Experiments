#version 150

in vec2 uv;

out vec4 color;

uniform float gamma;
uniform vec2 dxy;
uniform vec2 Nxy; // grid size

float E(float rho, float u, float v, float gamma, float p){
    return 0.5*rho*(u*u+v*v)+p/(gamma-1.0);
}

vec4 evaluateAt(float gamma, vec2 pos){
    vec4 value = vec4(0.0,0.0,0.0,0.0);
    
    value.x = 1.0;
    value.w = E(value.x,0.0,0.0,gamma, 1.0);
    
    const vec2 center = vec2(0.3,0.5);
    const float radius = 0.2;
    
    if(distance(pos, center) <= radius){
        value.x = 0.1;
        value.w = E(value.x,0.0,0.0,gamma,1.0);
    }else if(gl_FragCoord.x == 0.5){
        value.x = 3.81250;
        value.y = value.x*2.57669;
        value.w = E(value.x,2.57669,0.0,gamma,10.0);
    }
    
    return value;
}

void main() {
    float xfac  = 0.28867513459481288225*dxy.x;
    float yfac  = 0.28867513459481288225*dxy.y;
    
    vec2 pos  = ((gl_FragCoord.xy-0.5)/Nxy);
    vec2 pos0 = vec2(pos.x-xfac, pos.y-yfac);
    vec2 pos1 = vec2(pos.x+xfac, pos.y-yfac);
    vec2 pos2 = vec2(pos.x-xfac, pos.y+yfac);
    vec2 pos3 = vec2(pos.x+xfac, pos.y+yfac);
    
    vec4 value0 = evaluateAt(gamma, pos0);
    vec4 value1 = evaluateAt(gamma, pos1);
    vec4 value2 = evaluateAt(gamma, pos2);
    vec4 value3 = evaluateAt(gamma, pos3);

    color = (value0+value1+value2+value3)*0.25;
}