#version 150

in vec2 uv;

out vec4 color;

uniform sampler2D gradients;
uniform float maxGrad;

void main() {
    float grad  = texture(gradients,uv).x;
    float schlieren = pow((1.0-grad/2e-3f),55.0);
    
    color = vec4(vec3(schlieren),1.0);
    //color = texture(gradients,uv);
}