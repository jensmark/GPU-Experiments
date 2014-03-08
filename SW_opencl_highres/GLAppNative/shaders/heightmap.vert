#version 150

in vec2 position;

uniform mat4 projection_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;

uniform sampler2D tex;

out vec2 uv;

void main() {
    uv = position;
    float h = texture(tex, uv).x*0.5;
    gl_Position = projection_matrix*view_matrix*model_matrix*vec4(position.x, h, position.y, 1.0);
}