#version 400 core

uniform mat4 proj_matrix;
uniform mat4 modelview_matrix;

in vec3 position;

void main() {
	gl_Position = proj_matrix*modelview_matrix*vec4(position, 1.0);
}