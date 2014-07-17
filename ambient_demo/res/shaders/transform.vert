#version 150
uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;
uniform mat3 normal_matrix;

in vec3 position;
in vec3 normal;
in vec2 uv;

smooth out vec3 V;
smooth out vec3 L;
smooth out vec2 UV;
smooth out vec3 N;

void main() {
	vec4 pos = modelview_matrix * vec4(position, 1.0);
	V = normalize(-pos.xyz);
	L = normalize(vec3(10.0f, 10.0f, 10.0f) - pos.xyz);
	gl_Position = projection_matrix * pos;
	UV = uv;
	N = normal_matrix*normal;
}