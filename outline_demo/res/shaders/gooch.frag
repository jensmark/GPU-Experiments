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
    
    vec3 diffuse  = texture(diffuse_map,UV).xyz;
	vec3 kcool    = min(vec3(0.0,0.0,0.3) + diffuse, vec3(1.0));
    vec3 kwarm    = min(vec3(0.3,0.2,0.0) + diffuse, vec3(1.0));
    vec3 kfinal   = mix(kcool, kwarm, max(dot(n,l),0.0));
    
    vec3 spec     = texture(specular_map,UV).xyz;
    
    out_color = vec4(texture(ambient_map,UV).xyz+min(kfinal + spec, 1.0), 1.0);
}