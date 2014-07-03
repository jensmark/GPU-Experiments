#version 400 core

uniform vec3 diffuse = vec3(0.5,0.5,0.5);
uniform vec3 specular = vec3(0.5,0.5,0.5);
uniform vec3 ambient = vec3(1.0,0.0,0.0);
uniform float shininess = 25.0;
uniform float transparency = 1.0;

smooth in vec3 n;
smooth in vec3 v;
smooth in vec3 l;

out vec4 out_color;

void main() {
    vec3 h = normalize(v+l);
    vec3 n = normalize(n);
    
	vec4 diff = max(0.1f, dot(n, l)) * vec4(diffuse, transparency);
    vec4 spec = pow(max(0.0f, dot(n, h)), shininess) * vec4(specular, transparency);

    out_color = diff + spec + vec4(ambient, transparency);
}