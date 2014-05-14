/***
 * Function dec
 ****/
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
 * Set boundary conditions
 *
 ****/
__kernel void setBoundsX(__global float4* Q){
    unsigned int i = get_global_id(0);
    
    unsigned int Nx0 = Nx+4;
    unsigned int Ny0 = Ny+4;
    
    unsigned int k0 = (Nx0 * 0 + (i+2));
    unsigned int k1 = (Nx0 * 1 + (i+2));
    unsigned int k2 = (Nx0 * 2 + (i+2));
    unsigned int k3 = (Nx0 * 3 + (i+2));
    Q[k0] = Q[k1] = Q[k2];
    //Q[k0] = (float4)(Q[k3].x,Q[k3].y,-Q[k3].z,Q[k3].w);
    //Q[k1] = (float4)(Q[k2].x,Q[k2].y,-Q[k2].z,Q[k2].w);
    
    k0 = (Nx0 * (Ny0-1) + (i+2));
    k1 = (Nx0 * (Ny0-2) + (i+2));
    k2 = (Nx0 * (Ny0-3) + (i+2));
    k3 = (Nx0 * (Ny0-4) + (i+2));
    Q[k0] = Q[k1] = Q[k2];
    //Q[k0] = (float4)(Q[k3].x,Q[k3].y,-Q[k3].z,Q[k3].w);
    //Q[k1] = (float4)(Q[k2].x,Q[k2].y,-Q[k2].z,Q[k2].w);
}
__kernel void setBoundsY(__global float4* Q){
    unsigned int i = get_global_id(0);
    
    unsigned int Nx0 = Nx+4;
    unsigned int Ny0 = Ny+4;

    unsigned int k0 = (Nx0 * (i+2) + 0);
    unsigned int k1 = (Nx0 * (i+2) + 1);
    unsigned int k2 = (Nx0 * (i+2) + 2);
    unsigned int k3 = (Nx0 * (i+2) + 3);
    Q[k0] = Q[k1] = Q[k2];
    //Q[k0] = (float4)(Q[k3].x,-Q[k3].y,Q[k3].z,Q[k3].w);
    //Q[k1] = (float4)(Q[k2].x,-Q[k2].y,Q[k2].z,Q[k2].w);
    
    k0 = (Nx0 * (i+2) + (Nx0-1));
    k1 = (Nx0 * (i+2) + (Nx0-2));
    k2 = (Nx0 * (i+2) + (Nx0-3));
    k3 = (Nx0 * (i+2) + (Nx0-4));
    Q[k0] = Q[k1] = Q[k2];
    //Q[k0] = (float4)(Q[k3].x,-Q[k3].y,Q[k3].z,Q[k3].w);
    //Q[k1] = (float4)(Q[k2].x,-Q[k2].y,Q[k2].z,Q[k2].w);
}
