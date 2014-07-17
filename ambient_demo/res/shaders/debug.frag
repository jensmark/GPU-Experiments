#version 150

uniform sampler2D ambient_map;
uniform sampler2D diffuse_map;
uniform sampler2D specular_map;
uniform sampler2D normal_map;
uniform sampler2D view_map;
uniform sampler2D light_map;
uniform sampler2D depth_map;

in vec2 UV;

out vec4 out_color;

void main() {
    out_color = texture(depth_map,UV);
}