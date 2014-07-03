#version 400 core

uniform sampler2D depthmap;
in vec2 uv;

out vec4 color;

void main() {
    const float zNear = 0.01;
    const float zFar = 1.0;
    
    float z_b = textureLod(depthmap, uv, 3).x;
    float z_n = 2.0 * z_b - 1.0;
    float z_e = 2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear));
    
    color = vec4(z_e);
}