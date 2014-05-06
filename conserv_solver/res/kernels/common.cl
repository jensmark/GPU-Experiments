/***
 * Function dec
 ****/
float4  minmod(float4 a, float4 b);
float4  fetch(__global float4* array, unsigned int x, unsigned int y, unsigned int offset);
float   fetchf(__global float* array, unsigned int x, unsigned int y, unsigned int offset);
void    store(__global float4* array, float4 value, unsigned int x, unsigned int y, unsigned int offset);
void    storef(__global float* array, float value, unsigned int x, unsigned int y, unsigned int offset);

/****
 *
 * Utils
 *
 ****/
float4 fetch(__global float4* array, unsigned int x, unsigned int y, unsigned int offset){
    unsigned int k = ((Nx+4) * (y+offset) + (x+offset));
    return array[k];
}
float fetchf(__global float* array, unsigned int x, unsigned int y, unsigned int offset){
    unsigned int k = ((Nx+4) * (y+offset) + (x+offset));
    return array[k];
}
void store(__global float4* array, float4 value, unsigned int x, unsigned int y, unsigned int offset){
    unsigned int k = ((Nx+4) * (y+offset) + (x+offset));
    array[k] = value;
}
void storef(__global float* array, float value, unsigned int x, unsigned int y, unsigned int offset){
    unsigned int k = ((Nx+4) * (y+offset) + (x+offset));
    array[k] = value;
}

/****
 *
 * Perform piecewise polynominal reconstruction
 *
 ****/
float4 minmod(float4 a, float4 b){
    float4 res = min(fabs(a),fabs(b));
    return res*(sign(a)+sign(b))*0.5f;
}

__kernel void piecewiseReconstruction(__global float4* Q_in,
                                      __global float4* Sx_out, __global float4* Sy_out){
    unsigned int x = get_global_id(0);
    unsigned int y = get_global_id(1);
    
    float4 Q    = fetch(Q_in,x,y,1);
    float4 QE   = fetch(Q_in,x+1,y,1);
    float4 QW   = fetch(Q_in,x-1,y,1);
    float4 QN   = fetch(Q_in,x,y+1,1);
    float4 QS   = fetch(Q_in,x,y-1,1);
    
    store(Sx_out, minmod(Q-QW,QE-Q), x, y, 1);
    store(Sy_out, minmod(Q-QS,QN-Q), x, y, 1);
}

/****
 *
 * Compute one Runge-kutta step
 *
 ****/
__kernel void computeRK(__global float4* Q_in, __global float4* Qk_in, __global float4* F_in,
                        __global float4* G_in, float2 c, float2 dXY, float dT, __global float4* Q_out){
    unsigned int x = get_global_id(0);
    unsigned int y = get_global_id(1);
    
    float4 FE   = fetch(F_in,x,y,2);
    float4 FW   = fetch(F_in,x-1,y,2);
    float4 GN   = fetch(G_in,x,y,2);
    float4 GS   = fetch(G_in,x,y-1,2);
    
    float4 L    = -((FE-FW)/dXY.x+(GN-GS)/dXY.y);
    
    float4 Q    = fetch(Q_in,x,y,2);
    float4 Qk   = fetch(Qk_in,x,y,2);
    
    float4 v    = c.x*Q+c.y*(Qk+dT*L);
    store(Q_out, v, x, y,2);
}

/****
 *
 * Copy
 *
 ****/
__kernel void copy(__global float4* Q_in, __global float4* Q_out){
    unsigned int x = get_global_id(0);
    unsigned int y = get_global_id(1);
    
    // copy with ghost cells
    size_t k = ((Nx+4) * y + x);
    Q_out[k] = Q_in[k];
}

/****
 *
 * Prepare for visualization
 *
 ****/
__kernel void copyToTexture(__global float4* Q_in, __write_only image2d_t tex_out){
    unsigned int x = get_global_id(0);
    unsigned int y = get_global_id(1);
    
    size_t k = ((Nx+4) * (y+2) + (x+2));
    write_imagef(tex_out, (int2)(x,y), Q_in[k]);
}