#version 150

uniform vec3 diffuse;
uniform vec3 specular;
uniform vec3 ambient;
uniform float shininess;

uniform sampler2D main_tex;

smooth in vec3 V;
smooth in vec3 L;
smooth in vec2 UV;
smooth in vec3 N;

out vec4 out_color[6];

void main() {
    vec3 l = normalize(L);
    vec3 v = normalize(V);
    vec3 h = normalize(v+l);
    vec3 n = normalize(N);
    
    out_color[0] = vec4(ambient,1.0);
    out_color[1] = vec4(diffuse, 1.0) * texture(main_tex, UV);
    out_color[2] = pow(max(0.0f, dot(n, h)), shininess) * vec4(specular, 1.0);
    out_color[3] = vec4(n, 1.0);
    out_color[4] = vec4(v, 1.0);
    out_color[5] = vec4(l, 1.0);
}