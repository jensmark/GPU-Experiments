#version 150

uniform sampler2D ambient_map;
uniform sampler2D diffuse_map;
uniform sampler2D specular_map;
uniform sampler2D normal_map;
uniform sampler2D light_map;
uniform sampler2D depth_map;

in vec2 UV;

out vec4 out_color;

bool isDiscontinuty(){
    const float nThreshold = 0.8;
    const float dThreshold = 0.003;
    
    vec3 nN = textureOffset(normal_map,UV,ivec2(0,1)).xyz;
    vec3 nS = textureOffset(normal_map,UV,ivec2(0,-1)).xyz;
    vec3 nE = textureOffset(normal_map,UV,ivec2(1,0)).xyz;
    vec3 nW = textureOffset(normal_map,UV,ivec2(-1,0)).xyz;
    if (dot(nS,nN) < nThreshold) {
        return true;
    }
    if (dot(nE,nW) < nThreshold) {
        return true;
    }
    
    float dN = textureOffset(depth_map,UV,ivec2(0,1)).x;
    float dS = textureOffset(depth_map,UV,ivec2(0,-1)).x;
    float dE = textureOffset(depth_map,UV,ivec2(1,0)).x;
    float dW = textureOffset(depth_map,UV,ivec2(-1,0)).x;
    if (dN > dS) {
        if ((dN - dS) > dThreshold) {
            return true;
        }
    }else{
        if ((dS - dN) > dThreshold) {
            return true;
        }
    }
    
    if (dW > dE) {
        if ((dW - dE) > dThreshold) {
            return true;
        }
    }else{
        if ((dE - dW) > dThreshold) {
            return true;
        }
    }
    
    return false;
}

void main() {
    if (isDiscontinuty()) {
        out_color = vec4(0.0);
        return;
    }
    
    vec3 l = normalize(texture(light_map,UV).xyz);
    vec3 n = normalize(texture(normal_map,UV).xyz);
    
    vec3 diffuse  = texture(diffuse_map,UV).xyz;
	vec3 kcool    = min(vec3(0.0,0.0,0.3) + diffuse, vec3(1.0));
    vec3 kwarm    = min(vec3(0.3,0.2,0.0) + diffuse, vec3(1.0));
    vec3 kfinal   = mix(kcool, kwarm, max(dot(n,l),0.0));
    
    vec3 spec     = texture(specular_map,UV).xyz;
    
    out_color = vec4(texture(ambient_map,UV).xyz+min(kfinal + spec, 1.0), 1.0);
}