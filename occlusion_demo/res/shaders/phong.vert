#version 400 core

uniform mat4 proj_matrix;
uniform mat4 modelview_matrix;
uniform mat3 normal_matrix;

uniform vec3 light_pos = vec3(10.0,10.0,10.0);

in vec3 position;
in vec3 normal;

smooth out vec3 v;
smooth out vec3 l;
smooth out vec3 n;


void main() {
	vec4 pos = modelview_matrix * vec4(position, 1.0);
    
	v = normalize(-pos.xyz);
	l = normalize(light_pos - pos.xyz);
	n = normal_matrix*normal;
    
    gl_Position = proj_matrix * pos;
}