#version 150

uniform sampler2D ambient_map;
uniform sampler2D diffuse_map;
uniform sampler2D specular_map;
uniform sampler2D normal_map;
uniform sampler2D light_map;

in vec2 UV;

out vec4 out_color;

void main() {
    vec3 l = normalize(texture(light_map,UV).xyz);
    vec3 n = normalize(texture(normal_map,UV).xyz);
    
	vec4 diff = max(0.1f, dot(n, l)) * texture(diffuse_map,UV);
    vec4 spec = texture(specular_map,UV);

    out_color = diff + spec + texture(ambient_map,UV);
}