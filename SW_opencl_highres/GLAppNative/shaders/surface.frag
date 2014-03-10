#version 150

in vec2 uv;

out vec4 color;

uniform sampler2D tex;
uniform vec2 dxy;
uniform vec2 h_minmax;


vec4 HSV2RGB(vec4 hsva) {
    vec4 rgba;
    float h = hsva.x, s = hsva.y, v = hsva.z, m, n, f;
    float i;
    if( h == 0.0 )
        rgba = vec4(v, v, v, hsva.a);
    else {
        i = floor(h);
        f = h - i;
        float t = i / 2.0;
        if( t - floor( t ) <  0.1 )
            f = 1.0 - f; // if i is even
        m = v * (1.0 - s);
        n = v * (1.0 - s * f);
        if(i == 0.0 )       rgba = vec4(v, n, m, hsva.a);
        else if( i == 1.0 ) rgba = vec4(n, v, m, hsva.a);
        else if( i == 2.0 ) rgba = vec4(m, v, n, hsva.a);
        else if( i == 3.0 ) rgba = vec4(m, n, v, hsva.a);
        else if( i == 4.0 ) rgba = vec4(n, m, v, hsva.a);
        else                rgba = vec4(v, m, n, hsva.a);
    }
    return rgba; 
}

void main() {
    
    float n     = textureOffset(tex,uv,ivec2(0,1)).x;
    float s     = textureOffset(tex,uv,ivec2(0,-1)).x;
    float e     = textureOffset(tex,uv,ivec2(1,0)).x;
    float w 	= textureOffset(tex,uv,ivec2(-1,0)).x;
    vec3 a      = vec3(2.0*dxy.x, (e-w), 0.0);
    vec3 b      = vec3(0.0, (n-s), 2.0*dxy.y);
    vec3 norm 	= normalize(cross(b,a));
    
    const vec4 hsv1 = vec4(2.09, 1, 1, 1);
	const vec4 hsv2 = vec4(0.001, 1, 1, 1);
    
    vec4 c = texture(tex,uv);
    
    float sp    = 2.0*(abs(c.y/c.x)+abs(c.z/c.x));
    float t     = min((sp-h_minmax.x)/(h_minmax.y-h_minmax.x), 1.0);
	vec4 hsv    = mix(hsv1,hsv2,t);
    
    color = HSV2RGB(hsv)*max(dot(norm,vec3(0.5,1.0,0.5)),0.2);
}