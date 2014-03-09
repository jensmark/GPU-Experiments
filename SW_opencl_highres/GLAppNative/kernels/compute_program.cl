/***
 * Function dec
 ****/
float3  evaluateAt(float g, float2 pos);
float3  fflux(float g, float3 Q);
float3  gflux(float g, float3 Q);
float3  xFlux(float k, float g, float3 Q, float3 Q1, float3 Sx, float3 Sy, float3 Sxp, float3 Syp);
float3  yFlux(float k, float g, float3 Q, float3 Q1, float3 Sx, float3 Sy, float3 Sxp, float3 Syp);
float3  minmod(float3 a, float3 b);
float3  fetch(__global float3* array, unsigned int x, unsigned int y);
float   fetchf(__global float* array, unsigned int x, unsigned int y);
void    store(__global float3* array, float3 value, unsigned int x, unsigned int y);
void    storef(__global float* array, float value, unsigned int x, unsigned int y);

/****
 *
 * Utils
 *
 ****/
float3 fetch(__global float3* array, unsigned int x, unsigned int y){
    unsigned int k = ((Nx+4) * (y+2) + (x+2));
    return array[k];
}
float fetchf(__global float* array, unsigned int x, unsigned int y){
    unsigned int k = ((Nx+4) * (y+2) + (x+2));
    return array[k];
}
void store(__global float3* array, float3 value, unsigned int x, unsigned int y){
    unsigned int k = ((Nx+4) * (y+2) + (x+2));
    array[k] = value;
}
void storef(__global float* array, float value, unsigned int x, unsigned int y){
    unsigned int k = ((Nx+4) * (y+2) + (x+2));
    array[k] = value;
}

/****
 *
 * Evalulate numerical flux
 *
 ****/
float3 fflux(float g, float3 Q){
    if(Q.x <= 1.19e-07f){
        return (float3)(0.0f, 0.0f, 0.0f);
    }
    
    float k = 1e-10f*max(1.0f,min(1.0f/(float)(Nx),1.0f/(float)(Ny)));
    float u = 0.0f;
    if(Q.x < k){
        u = (sqrt(2.0f)*Q.x*Q.y)/(sqrt(pow(Q.x,4.0f)+max(pow(Q.x,4.0),k)));
    }else{
        u = Q.y/Q.x;
    }
    return (float3)(Q.y, (Q.y*u)+0.5f*g*(Q.x*Q.x), Q.z*u);
}

float3 gflux(float g, float3 Q){
    if(Q.x <= 1.19e-07f){
        return (float3)(0.0f, 0.0f, 0.0f);
    }
    
    float k = 1e-10f*max(1.0f,min(1.0f/(float)(Nx),1.0f/(float)(Ny)));
    float v = 0.0f;
    if(Q.x < k){
        v = (sqrt(2.0f)*Q.x*Q.z)/(sqrt(pow(Q.x,4.0f)+max(pow(Q.x,4.0),k)));
    }else{
        v = Q.z/Q.x;
    }
    return (float3)(Q.z, Q.y*v, (Q.z*v)+0.5f*g*(Q.x*Q.x));
}

float3 xFlux(float k, float g, float3 Q, float3 Q1, float3 Sx, float3 Sy, float3 Sxp, float3 Syp){
    float3 QW   = Q + Sx*0.5f;
    float3 QWp  = Q + Sx*0.5f + Sy*k;
    float3 QWm  = Q + Sx*0.5f - Sy*k;
    
    float3 QE   = Q1 - Sxp*0.5f;
    float3 QEp  = Q1 - Sxp*0.5f + Syp*k;
    float3 QEm  = Q1 - Sxp*0.5f - Syp*k;
    
    float c, ap, am;
    c           = sqrt(g*QW.x);
    ap          = max((QW.y+c)/QW.x,0.0f);
    am          = min((QW.y-c)/QW.x,0.0f);
    c           = sqrt(g*QE.x);
    ap          = max((QE.y+c)/QE.x,ap);
    am          = min((QE.y-c)/QE.x,am);
    
    float3 Fp   = ((ap*fflux(g, QWp) - am*fflux(g, QEp)) + (ap*am)*(QEp-QWp))/(ap-am);
    float3 Fm   = ((ap*fflux(g, QWm) - am*fflux(g, QEm)) + (ap*am)*(QEm-QWm))/(ap-am);
    return mix(Fp, Fm, 0.5f);
}

float3 yFlux(float k, float g, float3 Q, float3 Q1, float3 Sx, float3 Sy, float3 Sxp, float3 Syp){
    float3 QS   = Q + Sy*0.5f;
    float3 QSp  = Q + Sy*0.5f + Sx*k;
    float3 QSm  = Q + Sy*0.5f - Sx*k;
    
    float3 QN   = Q1 - Syp*0.5f;
    float3 QNp  = Q1 - Syp*0.5f + Sxp*k;
    float3 QNm  = Q1 - Syp*0.5f - Sxp*k;
    
    float c, ap, am;
    c           = sqrt(g*QS.x);
    ap          = max((QS.z+c)/QS.x,0.0f);
    am          = min((QS.z-c)/QS.x,0.0f);
    c           = sqrt(g*QN.x);
    ap          = max((QN.z+c)/QN.x,ap);
    am          = min((QN.z-c)/QN.x,am);
    
    float3 Gp   = ((ap*gflux(g, QSp) - am*gflux(g, QNp)) + (ap*am)*(QNp-QSp))/(ap-am);
    float3 Gm   = ((ap*gflux(g, QSm) - am*gflux(g, QNm)) + (ap*am)*(QNm-QSm))/(ap-am);
    return mix(Gp, Gm, 0.5f);
}

__kernel void computeNumericalFlux(__global float3* Q_in, __global float3* Sx_in, __global float3* Sy_in,
                                   float g, __global float3* F_out, __global float3* G_out){
    unsigned int x = get_global_id(0);
    unsigned int y = get_global_id(1);
    
    const float k = 0.2886751346f;
    
    float3 Q    = fetch(Q_in,x,y);
    float3 Sx   = fetch(Sx_in,x,y);
    float3 Sy   = fetch(Sy_in,x,y);
    
    float3 Q1   = fetch(Q_in,x+1,y);
    float3 Sxp  = fetch(Sx_in,x+1,y);
    float3 Syp  = fetch(Sy_in,x+1,y);
    
    store(F_out, xFlux(k, g, Q, Q1, Sx, Sy, Sxp, Syp), x, y);
    
    Q1   = fetch(Q_in,x,y+1);
    Sxp  = fetch(Sx_in,x,y+1);
    Syp  = fetch(Sy_in,x,y+1);
    
    store(G_out, yFlux(k, g, Q, Q1, Sx, Sy, Sxp, Syp), x, y);
}


/****
 *
 * Perform piecewise polynominal reconstruction
 *
 ****/
float3 minmod(float3 a, float3 b){
    float3 res = min(fabs(a),fabs(b));
    return res*(sign(a)+sign(b))*0.5f;
}

__kernel void piecewiseReconstruction(__global float3* Q_in,
                                      __global float3* Sx_out, __global float3* Sy_out){
    unsigned int x = get_global_id(0);
    unsigned int y = get_global_id(1);
    
    float3 Q    = fetch(Q_in,x,y);
    float3 QE   = fetch(Q_in,x+1,y);
    float3 QW   = fetch(Q_in,x-1,y);
    float3 QN   = fetch(Q_in,x,y+1);
    float3 QS   = fetch(Q_in,x,y-1);
    
    store(Sx_out, minmod(Q-QW,QE-Q), x, y);
    store(Sy_out, minmod(Q-QS,QN-Q), x, y);
}

/****
 *
 * Compute one Runge-kutta step
 *
 ****/
__kernel void computeRK(__global float3* Q_in, __global float3* Qk_in, __global float3* F_in,
                        __global float3* G_in, float2 c, float2 dXY, float dT, __global float3* Q_out){
    unsigned int x = get_global_id(0);
    unsigned int y = get_global_id(1);
    
    float3 FE   = fetch(F_in,x,y);
    float3 FW   = fetch(F_in,x-1,y);
    float3 GN   = fetch(G_in,x,y);
    float3 GS   = fetch(G_in,x,y-1);
    
    float3 L    = -((FE-FW)/dXY.x+(GN-GS)/dXY.y);
    
    float3 Q    = fetch(Q_in,x,y);
    float3 Qk   = fetch(Qk_in,x,y);
    
    float3 v    = c.x*Q+c.y*(Qk+dT*L);
    store(Q_out, v, x, y);
}


/****
 *
 * Set initial condition
 *
 ****/
float3 evaluateAt(float g, float2 pos){
    float3 value = (float3)(2.0f,0.0f,0.0f);
    
    value.x = value.x + 1.5*exp(-pow((2.0f*(pos.x-0.5f)),2.0f)/(2.0f*pow(0.3f,2.0f))
                            -pow((2.0f*(pos.y-0.5f)),2.0f)/(2.0f*pow(0.3f,2.0f)));
    
    return value;
}

__kernel void setInitial(float g, float2 dXY, __global float3* Q_out){
    unsigned int x = get_global_id(0);
    unsigned int y = get_global_id(1);
    
    float xfac  = 0.28867513459481288225f*dXY.x;
    float yfac  = 0.28867513459481288225f*dXY.y;
    
    float2 pos  = (float2)((float)x/(float)Nx,(float)y/(float)Ny);
    float2 pos0 = (float2)(pos.x-xfac, pos.y-yfac);
    float2 pos1 = (float2)(pos.x+xfac, pos.y-yfac);
    float2 pos2 = (float2)(pos.x-xfac, pos.y+yfac);
    float2 pos3 = (float2)(pos.x+xfac, pos.y+yfac);
    
    float3 value0 = evaluateAt(g, pos0);
    float3 value1 = evaluateAt(g, pos1);
    float3 value2 = evaluateAt(g, pos2);
    float3 value3 = evaluateAt(g, pos3);

    store(Q_out,(value0+value1+value2+value3)*0.25f,x,y);
}

/****
 *
 * Set boundary conditions
 *
 ****/
__kernel void setBoundsX(__global float3* Q){
    unsigned int i = get_global_id(0);
    
    unsigned int Nx0 = Nx+4;
    unsigned int Ny0 = Ny+4;
    
    unsigned int k0 = (Nx0 * 1 + i);
    unsigned int k1 = (Nx0 * 2 + i);
    unsigned int k2 = (Nx0 * 3 + i);
    //Q[k0] = Q[k1] = Q[k2];
    Q[k0] = Q[k1] = (float3)(Q[k2].x,Q[k2].y,-Q[k2].z);
    
    k0 = (Nx0 * (Ny0-1) + i);
    k1 = (Nx0 * (Ny0-2) + i);
    k2 = (Nx0 * (Ny0-3) + i);
    //Q[k0] = Q[k1] = Q[k2];
    Q[k0] = Q[k1] = (float3)(Q[k2].x,Q[k2].y,-Q[k2].z);
}
__kernel void setBoundsY(__global float3* Q){
    unsigned int i = get_global_id(0);
    
    unsigned int Nx0 = Nx+4;
    unsigned int Ny0 = Ny+4;

    unsigned int k0 = (Nx0 * i + 1);
    unsigned int k1 = (Nx0 * i + 2);
    unsigned int k2 = (Nx0 * i + 3);
    //Q[k0] = Q[k1] = Q[k2];
    Q[k0] = Q[k1] = (float3)(Q[k2].x,-Q[k2].y,Q[k2].z);
    
    k0 = (Nx0 * i + (Nx0-1));
    k1 = (Nx0 * i + (Nx0-2));
    k2 = (Nx0 * i + (Nx0-3));
    //Q[k0] = Q[k1] = Q[k2];
    Q[k0] = Q[k1] = (float3)(Q[k2].x,-Q[k2].y,Q[k2].z);
}

/****
 *
 * Compute eigenvalues
 *
 ****/
__kernel void eigenvalue(__global float3* Q_in, float g, __global float* E_out){
    unsigned int x = get_global_id(0);
    unsigned int y = get_global_id(1);
    
    float3 Q    = fetch(Q_in, x, y);
    float2 uv   = Q.yz/Q.x;
    float c     = sqrt(g*Q.x);
    
    float eigen;
    eigen = max(fabs(uv.x)-c,0.0f);
    eigen = max(fabs(uv.x)+c,fabs(eigen));
    eigen = max(fabs(uv.y)-c,fabs(eigen));
    eigen = max(fabs(uv.y)+c,fabs(eigen));
    
    storef(E_out, eigen, x, y);
}

/****
 *
 * Copy
 *
 ****/
__kernel void copy(__global float3* Q_in, __global float3* Q_out){
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
__kernel void copyToTexture(__global float3* Q_in, __write_only image2d_t tex_out){
    unsigned int x = get_global_id(0);
    unsigned int y = get_global_id(1);
    
    size_t k = ((Nx+4) * (y+2) + (x+2));
    write_imagef(tex_out, (int2)(x,y), (float4)(Q_in[k],1.0f));
}