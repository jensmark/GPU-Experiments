#version 400 core

uniform sampler2D lastMip;
uniform ivec2 lastMipSize;

in vec2 uv;

void main() {
    
    vec4 texels;
    texels.x = texture( lastMip, uv ).x;
    texels.y = textureOffset( lastMip, uv, ivec2(-1, 0) ).x;
    texels.z = textureOffset( lastMip, uv, ivec2(-1,-1) ).x;
    texels.w = textureOffset( lastMip, uv, ivec2( 0,-1) ).x;
    
    float maxZ = max( max( texels.x, texels.y ), max( texels.z, texels.w ) );
    
    vec3 extra;
    // if we are reducing an odd-width texture then fetch the edge texels
    if ( ( (lastMipSize.x & 1) != 0 ) && ( int(gl_FragCoord.x) == lastMipSize.x-3 ) ) {
        // if both edges are odd, fetch the top-left corner texel
        if ( ( (lastMipSize.y & 1) != 0 ) && ( int(gl_FragCoord.y) == lastMipSize.y-3 ) ) {
            extra.z = textureOffset( lastMip, uv, ivec2( 1, 1) ).x;
            maxZ = max( maxZ, extra.z );
        }
        extra.x = textureOffset( lastMip, uv, ivec2( 1, 0) ).x;
        extra.y = textureOffset( lastMip, uv, ivec2( 1,-1) ).x;
        maxZ = max( maxZ, max( extra.x, extra.y ) );
    } else
        // if we are reducing an odd-height texture then fetch the edge texels
        if ( ( (lastMipSize.y & 1) != 0 ) && ( int(gl_FragCoord.y) == lastMipSize.y-3 ) ) {
            extra.x = textureOffset( lastMip, uv, ivec2( 0, 1) ).x;
            extra.y = textureOffset( lastMip, uv, ivec2(-1, 1) ).x;
            maxZ = max( maxZ, max( extra.x, extra.y ) );
        }
    
    gl_FragDepth = maxZ;
    
}