#version 400 core

uniform mat4 proj_matrix;
uniform mat4 view_matrix;
uniform vec2 viewport;

uniform sampler2D hiZBuffer;

in vec3 position;

vec4 boundingBox[8];

flat out int visible;

void BuildBB(in vec3 pos){
    boundingBox[0] = proj_matrix*view_matrix*vec4(pos + vec3( 0.5, 0.5, 0.5), 1.0 );
    boundingBox[1] = proj_matrix*view_matrix*vec4(pos + vec3(-0.5, 0.5, 0.5), 1.0 );
    boundingBox[2] = proj_matrix*view_matrix*vec4(pos + vec3( 0.5,-0.5, 0.5), 1.0 );
    boundingBox[3] = proj_matrix*view_matrix*vec4(pos + vec3(-0.5,-0.5, 0.5), 1.0 );
    boundingBox[4] = proj_matrix*view_matrix*vec4(pos + vec3( 0.5, 0.5,-0.5), 1.0 );
    boundingBox[5] = proj_matrix*view_matrix*vec4(pos + vec3(-0.5, 0.5,-0.5), 1.0 );
    boundingBox[6] = proj_matrix*view_matrix*vec4(pos + vec3( 0.5,-0.5,-0.5), 1.0 );
    boundingBox[7] = proj_matrix*view_matrix*vec4(pos + vec3(-0.5,-0.5,-0.5), 1.0 );
}

int HiZOcclusionCull(in vec3 pos) {
	BuildBB(pos);
	
	/* perform perspective division for the bounding box */
    for (int i=0; i<8; i++)
		boundingBox[i].xyz /= boundingBox[i].w;
	
	/* calculate screen space bounding rectangle */
	vec2 boundingRect[2];
	boundingRect[0].x = min( min( min( boundingBox[0].x, boundingBox[1].x ),
                                 min( boundingBox[2].x, boundingBox[3].x ) ),
                            min( min( boundingBox[4].x, boundingBox[5].x ),
                                min( boundingBox[6].x, boundingBox[7].x ) ) ) / 2.0 + 0.5;
	boundingRect[0].y = min( min( min( boundingBox[0].y, boundingBox[1].y ),
                                 min( boundingBox[2].y, boundingBox[3].y ) ),
                            min( min( boundingBox[4].y, boundingBox[5].y ),
                                min( boundingBox[6].y, boundingBox[7].y ) ) ) / 2.0 + 0.5;
	boundingRect[1].x = max( max( max( boundingBox[0].x, boundingBox[1].x ),
                                 max( boundingBox[2].x, boundingBox[3].x ) ),
                            max( max( boundingBox[4].x, boundingBox[5].x ),
                                max( boundingBox[6].x, boundingBox[7].x ) ) ) / 2.0 + 0.5;
	boundingRect[1].y = max( max( max( boundingBox[0].y, boundingBox[1].y ),
                                 max( boundingBox[2].y, boundingBox[3].y ) ),
                            max( max( boundingBox[4].y, boundingBox[5].y ),
                                max( boundingBox[6].y, boundingBox[7].y ) ) ) / 2.0 + 0.5;
	/* then the linear depth value of the front-most point */
	float depth = min( min( min( boundingBox[0].z, boundingBox[1].z ),
                                   min( boundingBox[2].z, boundingBox[3].z ) ),
                              min( min( boundingBox[4].z, boundingBox[5].z ),
                                  min( boundingBox[6].z, boundingBox[7].z ) ) );
    
	/* now we calculate the bounding rectangle size in viewport coordinates */
	float viewSizeX = (boundingRect[1].x-boundingRect[0].x) * viewport.x;
	float viewSizeY = (boundingRect[1].y-boundingRect[0].y) * viewport.y;
	
	/* now we calculate the texture LOD used for lookup in the depth buffer texture */
	float LOD = ceil( log2( max( viewSizeX, viewSizeY ) / 2.0 ) );
	
	/* finally fetch the depth texture using explicit LOD lookups */
	vec4 samples;
	samples.x = textureLod( hiZBuffer, vec2(boundingRect[0].x, boundingRect[0].y), LOD ).x;
	samples.y = textureLod( hiZBuffer, vec2(boundingRect[0].x, boundingRect[1].y), LOD ).x;
	samples.z = textureLod( hiZBuffer, vec2(boundingRect[1].x, boundingRect[1].y), LOD ).x;
	samples.w = textureLod( hiZBuffer, vec2(boundingRect[1].x, boundingRect[0].y), LOD ).x;
	float maxDepth = max( max( samples.x, samples.y ), max( samples.z, samples.w ) );
	
	/* if the instance depth is bigger than the depth in the texture discard the instance */
	return ( depth > maxDepth ) ? 0 : 1;
}

void main() {
	visible = HiZOcclusionCull(position);
    gl_Position = vec4(position,1.0);
}