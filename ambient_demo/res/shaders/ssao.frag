#version 150

uniform mat4 projection_matrix;

uniform sampler2D view_map;
uniform sampler2D normal_map;
uniform sampler2D depth_map;
uniform sampler2D noise_map;

uniform vec2 noise_scale;
uniform float radius;

const int sample_count = 16;
uniform vec3 samples[16];


in vec2 UV;

out vec4 out_color;

float linearizeDepth(in float depth, in mat4 projMatrix) {
	return projMatrix[3][2] / (depth - projMatrix[2][2]);
}

void main() {
    vec3 v = normalize(texture(view_map,UV).xyz);
    vec3 origin = v * linearizeDepth(texture(depth_map,UV).x,
                                     projection_matrix);
    vec3 n = normalize(texture(normal_map,UV).xyz);
    
    vec3 rvec = texture(noise_map, UV * noise_scale).xyz;
    vec3 tangent = normalize(rvec - n * dot(rvec, n));
    vec3 bitangent = cross(n, tangent);
    mat3 tbn = mat3(tangent, bitangent, n);

    float occlusion = 0.0;
    for (int i = 0; i < sample_count; i++) {
        // get sample position:
        vec3 sample = tbn * samples[i];
        sample = sample * radius + origin;
        
        // project sample position:
        vec4 offset = vec4(sample, 1.0);
        offset = projection_matrix * offset;
        offset.xy /= offset.w;
        offset.xy = offset.xy * 0.5 + 0.5;
        
        // get sample depth:
        float sampleDepth = linearizeDepth(texture(depth_map, offset.xy).x,
                                           projection_matrix);
        
        // range check & accumulate:
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(origin.z - sampleDepth));
		occlusion += rangeCheck * step(sampleDepth, sample.z);
    }
    occlusion = 1.0 - (occlusion / sample_count);
    out_color = vec4(pow(occlusion,0.3));
}