/****
 *
 * Evalulate numerical flux
 *
 ****/
float pressure(float4 Q){
    return (gamma−1.0)*(Q.w − 0.5*dot(Q.yz,Q.yz)/Q.x);
}

float4 fflux(float4 Q){
    float u = Q.y/Q.x;
    float p = pressure(Q);
    return vec4(Q.y, (Q.y*u)+p, Q.z*u, u*(Q.w+p) );
}

float4 gflux(float4 Q){
    float v = Q.z/Q.x;
    float p = pressure(Q);
    return vec4(Q.z, (Q.z*v)+p, Q.y*v, v*(Q.w+p));
}

float4 xReconstruction(){
    return (float4)(0.0,0.0,0.0,0.0);
}

float4 yReconstruction(){
    return (float4)(0.0,0.0,0.0,0.0);
}

__kernel void computeNumericalFlux(__global float4* Q_in, __global float4* Sx_in, __global float4* Sy_in,
                                   float gamma, __global float4* F_out, __global float4* G_out){
    unsigned int x = get_global_id(0);
    unsigned int y = get_global_id(1);
    
    size_t i = x*get_global_size(0)+y;
    F_out[i] = xReconstruction();
    G_out[i] = yReconstruction();
}


/****
 *
 * Perform piecewise polynominal reconstruction
 *
 ****/
float4 minmod(float4 a, float4 b){
    float4 res = min(abs(a),abs(b));
    return res*(sign(a)+sign(b))*0.5;
}

__kernel void piecewiseReconstruction(__global float4* Q_in, float2 dXY,
                                      __global float4* Sx_out, __global float4* Sy_out){
    unsigned int x = get_global_id(0);
    unsigned int y = get_global_id(1);
    
    
}

/****
 *
 * Compute one Runge-kutta step
 *
 ****/
__kernel void computeRK(__global float4* Q_in, __global float4* F_in, __global float4* G_in,
                        float2 c, float2 dXY, float dT, __global float4* Q_out){
    unsigned int x = get_global_id(0);
    unsigned int y = get_global_id(1);
    
    
}

/****
 *
 * Prepare for visualization
 *
 ****/
__kernel void copyToTexture(__global float4* Q_in, __global image2d_t* tex_out){
    unsigned int x = get_global_id(0);
    unsigned int y = get_global_id(1);
    
    size_t i = x*get_global_size(0)+y;
    write_imagef(tex_out, (int2)(x,y), Q_in[i]);
}