/***
 * Function dec
 ****/
float   pressure(float gamma, float4 Q);
float   energy(float gamma, float4 Q, float p);
float4  evaluateAt(float gamma, float2 pos);
float4  fflux(float gamma, float4 Q);
float4  gflux(float gamma, float4 Q);
float4  xFlux(float k, float gamma, float4 Q, float4 Q1, float4 Sx, float4 Sy, float4 Sxp, float4 Syp);
float4  yFlux(float k, float gamma, float4 Q, float4 Q1, float4 Sx, float4 Sy, float4 Sxp, float4 Syp);
float4  minmod(float4 a, float4 b);
float4  fetch(__global float4* array, unsigned int x, unsigned int y);
float   fetchf(__global float* array, unsigned int x, unsigned int y);
void    store(__global float4* array, float4 value, unsigned int x, unsigned int y);
void    storef(__global float* array, float value, unsigned int x, unsigned int y);

/****
 *
 * Utils
 *
 ****/
float4 fetch(__global float4* array, unsigned int x, unsigned int y){
    unsigned int k = ((Nx+4) * (y+2) + (x+2));
    return array[k];
}
float fetchf(__global float* array, unsigned int x, unsigned int y){
    unsigned int k = ((Nx+4) * (y+2) + (x+2));
    return array[k];
}
void store(__global float4* array, float4 value, unsigned int x, unsigned int y){
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
float pressure(float gamma, float4 Q){
    return (gamma-1.0f)*(Q.w-0.5f*dot(Q.yz,Q.yz)/Q.x);
}

float4 fflux(float gamma, float4 Q){
    float u = Q.y/Q.x;
    float p = pressure(gamma, Q);
    return (float4)(Q.y, (Q.y*u)+p, Q.z*u, u*(Q.w+p));
}

float4 gflux(float gamma, float4 Q){
    float v = Q.z/Q.x;
    float p = pressure(gamma, Q);
    return (float4)(Q.z, Q.y*v, (Q.z*v)+p, v*(Q.w+p));
}

float4 xFlux(float k, float gamma, float4 Q, float4 Q1, float4 Sx, float4 Sy, float4 Sxp, float4 Syp){
    float4 QW   = Q + Sx*0.5f;
    float4 QWp  = Q + Sx*0.5f + Sy*k;
    float4 QWm  = Q + Sx*0.5f - Sy*k;
    
    float4 QE   = Q1 - Sxp*0.5f;
    float4 QEp  = Q1 - Sxp*0.5f + Syp*k;
    float4 QEm  = Q1 - Sxp*0.5f - Syp*k;
    
    float c, ap, am;
    c           = sqrt(gamma*QW.x*pressure(gamma, QW));
    ap          = max((QW.y+c)/QW.x,0.0f);
    am          = min((QW.y-c)/QW.x,0.0f);
    c           = sqrt(gamma*QE.x*pressure(gamma, QE));
    ap          = max((QE.y+c)/QE.x,ap);
    am          = min((QE.y-c)/QE.x,am);
    
    float4 Fp   = ((ap*fflux(gamma, QWp) - am*fflux(gamma, QEp)) + (ap*am)*(QEp-QWp))/(ap-am);
    float4 Fm   = ((ap*fflux(gamma, QWm) - am*fflux(gamma, QEm)) + (ap*am)*(QEm-QWm))/(ap-am);
    return mix(Fp, Fm, 0.5f);
}

float4 yFlux(float k, float gamma, float4 Q, float4 Q1, float4 Sx, float4 Sy, float4 Sxp, float4 Syp){    
    float4 QS   = Q + Sy*0.5f;
    float4 QSp  = Q + Sy*0.5f + Sx*k;
    float4 QSm  = Q + Sy*0.5f - Sx*k;
    
    float4 QN   = Q1 - Syp*0.5f;
    float4 QNp  = Q1 - Syp*0.5f + Sxp*k;
    float4 QNm  = Q1 - Syp*0.5f - Sxp*k;
    
    float c, ap, am;
    c           = sqrt(gamma*QS.x*pressure(gamma, QS));
    ap          = max((QS.z+c)/QS.x,0.0f);
    am          = min((QS.z-c)/QS.x,0.0f);
    c           = sqrt(gamma*QN.x*pressure(gamma, QN));
    ap          = max((QN.z+c)/QN.x,ap);
    am          = min((QN.z-c)/QN.x,am);
    
    float4 Gp   = ((ap*gflux(gamma, QSp) - am*gflux(gamma, QNp)) + (ap*am)*(QNp-QSp))/(ap-am);
    float4 Gm   = ((ap*gflux(gamma, QSm) - am*gflux(gamma, QNm)) + (ap*am)*(QNm-QSm))/(ap-am);
    return mix(Gp, Gm, 0.5f);
}

__kernel void computeNumericalFlux(__global float4* Q_in, __global float4* Sx_in, __global float4* Sy_in,
                                   float gamma, __global float4* F_out, __global float4* G_out){
    unsigned int x = get_global_id(0);
    unsigned int y = get_global_id(1);
    
    const float k = 0.2886751346f;
    
    float4 Q    = fetch(Q_in,x,y);
    float4 Sx   = fetch(Sx_in,x,y);
    float4 Sy   = fetch(Sy_in,x,y);
    
    float4 Q1   = fetch(Q_in,x+1,y);
    float4 Sxp  = fetch(Sx_in,x+1,y);
    float4 Syp  = fetch(Sy_in,x+1,y);
    
    store(F_out, xFlux(k, gamma, Q, Q1, Sx, Sy, Sxp, Syp), x, y);
    
    Q1   = fetch(Q_in,x,y+1);
    Sxp  = fetch(Sx_in,x,y+1);
    Syp  = fetch(Sy_in,x,y+1);
    
    store(G_out, yFlux(k, gamma, Q, Q1, Sx, Sy, Sxp, Syp), x, y);
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
    
    float4 Q    = fetch(Q_in,x,y);
    float4 QE   = fetch(Q_in,x+1,y);
    float4 QW   = fetch(Q_in,x-1,y);
    float4 QN   = fetch(Q_in,x,y+1);
    float4 QS   = fetch(Q_in,x,y-1);
    
    store(Sx_out, minmod(Q-QW,QE-Q), x, y);
    store(Sy_out, minmod(Q-QS,QN-Q), x, y);
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
    
    float4 FE   = fetch(F_in,x,y);
    float4 FW   = fetch(F_in,x-1,y);
    float4 GN   = fetch(G_in,x,y);
    float4 GS   = fetch(G_in,x,y-1);
    
    float4 L    = -((FE-FW)/dXY.x+(GN-GS)/dXY.y);
    
    float4 Q    = fetch(Q_in,x,y);
    float4 Qk   = fetch(Qk_in,x,y);
    
    store(Q_out, c.x*Q+c.y*(Qk+dT*L), x, y);
}


/****
 *
 * Set initial condition
 *
 ****/
float E(float rho, float u, float v, float gamma, float p){
    return 0.5f*rho*(u*u+v*v)+p/(gamma-1.0f);
}

float4 evaluateAt(float gamma, float2 pos){
    float4 value = (float4)(0.0f,0.0f,0.0f,0.0f);
    
    value.x = 1.0f;
    value.w = E(value.x, 0.0f, 0.0f, gamma, 1.0f);
    
    const float2 center = (float2)(0.3f,0.5f);
    const float radius = 0.2f;
    
    if(distance(pos, center) <= radius){
        value.x = 0.1f;
        value.w = E(value.x, 0.0f, 0.0f, gamma, 1.0f);
    }else if(pos.x <= 0.01f){
        value.x = 3.81250f;
        value.y = value.x*2.57669250441241f;
        value.w = E(value.x, value.y/value.x, 0.0f, gamma, 10.0f);
    }
    
    return value;
}

__kernel void setInitial(float gamma, float2 dXY, __global float4* Q_out){
    unsigned int x = get_global_id(0);
    unsigned int y = get_global_id(1);
    
    float xfac  = 0.28867513459481288225f*dXY.x;
    float yfac  = 0.28867513459481288225f*dXY.y;
    
    float2 pos  = (float2)((float)x/(float)Nx,(float)y/(float)Ny);
    float2 pos0 = (float2)(pos.x-xfac, pos.y-yfac);
    float2 pos1 = (float2)(pos.x+xfac, pos.y-yfac);
    float2 pos2 = (float2)(pos.x-xfac, pos.y+yfac);
    float2 pos3 = (float2)(pos.x+xfac, pos.y+yfac);
    
    float4 value0 = evaluateAt(gamma, pos0);
    float4 value1 = evaluateAt(gamma, pos1);
    float4 value2 = evaluateAt(gamma, pos2);
    float4 value3 = evaluateAt(gamma, pos3);

    store(Q_out,(value0+value1+value2+value3)*0.25f,x,y);
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
    
    unsigned int k0 = (Nx0 * 1 + i);
    unsigned int k1 = (Nx0 * 2 + i);
    unsigned int k2 = (Nx0 * 3 + i);
    Q[k0] = Q[k1] = Q[k2];
    
    k0 = (Nx0 * (Ny0-1) + i);
    k1 = (Nx0 * (Ny0-2) + i);
    k2 = (Nx0 * (Ny0-3) + i);
    Q[k0] = Q[k1] = Q[k2];
}
__kernel void setBoundsY(__global float4* Q){
    unsigned int i = get_global_id(0);
    
    unsigned int Nx0 = Nx+4;
    unsigned int Ny0 = Ny+4;

    unsigned int k0 = (Nx0 * i + 1);
    unsigned int k1 = (Nx0 * i + 2);
    unsigned int k2 = (Nx0 * i + 3);
    Q[k0] = Q[k1] = Q[k2];
    
    k0 = (Nx0 * i + (Nx0-1));
    k1 = (Nx0 * i + (Nx0-2));
    k2 = (Nx0 * i + (Nx0-3));
    Q[k0] = Q[k1] = Q[k2];
}

/****
 *
 * Compute eigenvalues
 *
 ****/
__kernel void eigenvalue(__global float4* Q_in, float gamma, __global float* E_out){
    unsigned int x = get_global_id(0);
    unsigned int y = get_global_id(1);
    
    float4 Q    = fetch(Q_in, x, y);
    float2 uv   = Q.yz/Q.x;
    float c     = sqrt(gamma*pressure(gamma,Q)/Q.x);
    
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
    //write_imagef(tex_out, (int2)(x,y), (float4)(1.0f,0.0f,1.0f,1.0f));
}