#version 150

in vec2 uv;

out vec4 color;

uniform sampler2D QTex;

uniform float gamma;

float P(in vec4 Q){
    return (gamma-1.0)*(Q.w-0.5*dot(Q.yz,Q.yz));
}

void main() {
    vec4 Q  = texture(QTex, uv);
    Q.yz /= Q.x;
    float c = sqrt(gamma*P(Q)/Q.x);
    
    float eigen;
    eigen = max(abs(Q.y-c),0.0);
    eigen = max(abs(Q.y+c),abs(eigen));
    eigen = max(abs(Q.z-c),abs(eigen));
    eigen = max(abs(Q.z+c),abs(eigen));
    
    color = vec4(eigen);
}