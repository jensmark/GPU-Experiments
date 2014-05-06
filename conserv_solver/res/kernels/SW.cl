/***
 * Function dec
 ****/
float4  fflux(float g, float4 Q);
float4  gflux(float g, float4 Q);
float4  xFlux(float k, float g, float4 Q, float4 Q1, float4 Sx, float4 Sy, float4 Sxp, float4 Syp);
float4  yFlux(float k, float g, float4 Q, float4 Q1, float4 Sx, float4 Sy, float4 Sxp, float4 Syp);
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
 * Evalulate numerical flux
 *
 ****/
float4 fflux(float g, float4 Q){
    if(Q.x <= 1.19e-07f){
        return (float4)(0.0f, 0.0f, 0.0f,0.0f);
    }
    
    float k = 1e-1f*max(1.0f,min(1.0f/(float)(Nx),1.0f/(float)(Ny)));
    float u = 0.0f;
    if(Q.x < k){
        u = (sqrt(2.0f)*Q.x*Q.y)/(sqrt(pow(Q.x,4.0f)+max(pow(Q.x,4.0f),k)));
    }else{
        u = Q.y/Q.x;
    }
    return (float4)(Q.y, (Q.y*u)+(0.5f*g*Q.x*Q.x), Q.z*u,0.0f);
}

float4 gflux(float g, float4 Q){
    if(Q.x <= 1.19e-07f){
        return (float4)(0.0f, 0.0f, 0.0f,0.0f);
    }
    
    float k = 1e-1f*max(1.0f,min(1.0f/(float)(Nx),1.0f/(float)(Ny)));
    float v = 0.0f;
    if(Q.x < k){
        v = (sqrt(2.0f)*Q.x*Q.z)/(sqrt(pow(Q.x,4.0f)+max(pow(Q.x,4.0f),k)));
    }else{
        v = Q.z/Q.x;
    }
    return (float4)(Q.z, Q.y*v, (Q.z*v)+(0.5f*g*Q.x*Q.x),0.0f);
}

float4 xFlux(float k, float g, float4 Q, float4 Q1, float4 Sx, float4 Sy, float4 Sxp, float4 Syp){
    float4 QW   = Q + Sx*0.5f;
    float4 QWp  = Q + Sx*0.5f + Sy*k;
    float4 QWm  = Q + Sx*0.5f - Sy*k;
    
    float4 QE   = Q1 - Sxp*0.5f;
    float4 QEp  = Q1 - Sxp*0.5f + Syp*k;
    float4 QEm  = Q1 - Sxp*0.5f - Syp*k;
    
    float c, ap, am;
    c           = sqrt(g*QW.x);
    ap          = max((QW.y+c)/QW.x,0.0f);
    am          = min((QW.y-c)/QW.x,0.0f);
    c           = sqrt(g*QE.x);
    ap          = max((QE.y+c)/QE.x,ap);
    am          = min((QE.y-c)/QE.x,am);
    
    float4 Fp   = ((ap*fflux(g, QWp) - am*fflux(g, QEp)) + (ap*am)*(QEp-QWp))/(ap-am);
    float4 Fm   = ((ap*fflux(g, QWm) - am*fflux(g, QEm)) + (ap*am)*(QEm-QWm))/(ap-am);
    return mix(Fp, Fm, 0.5f);
}

float4 yFlux(float k, float g, float4 Q, float4 Q1, float4 Sx, float4 Sy, float4 Sxp, float4 Syp){
    float4 QS   = Q + Sy*0.5f;
    float4 QSp  = Q + Sy*0.5f + Sx*k;
    float4 QSm  = Q + Sy*0.5f - Sx*k;
    
    float4 QN   = Q1 - Syp*0.5f;
    float4 QNp  = Q1 - Syp*0.5f + Sxp*k;
    float4 QNm  = Q1 - Syp*0.5f - Sxp*k;
    
    float c, ap, am;
    c           = sqrt(g*QS.x);
    ap          = max((QS.z+c)/QS.x,0.0f);
    am          = min((QS.z-c)/QS.x,0.0f);
    c           = sqrt(g*QN.x);
    ap          = max((QN.z+c)/QN.x,ap);
    am          = min((QN.z-c)/QN.x,am);
    
    float4 Gp   = ((ap*gflux(g, QSp) - am*gflux(g, QNp)) + (ap*am)*(QNp-QSp))/(ap-am);
    float4 Gm   = ((ap*gflux(g, QSm) - am*gflux(g, QNm)) + (ap*am)*(QNm-QSm))/(ap-am);
    return mix(Gp, Gm, 0.5f);
}

__kernel void computeNumericalFlux(__global float4* Q_in, __global float4* Sx_in, __global float4* Sy_in,
                                   float g, __global float4* F_out, __global float4* G_out){
    unsigned int x = get_global_id(0);
    unsigned int y = get_global_id(1);
    
    const float k = 0.2886751346f;
    
    float4 Q    = fetch(Q_in,x,y,1);
    float4 Sx   = fetch(Sx_in,x,y,1);
    float4 Sy   = fetch(Sy_in,x,y,1);
    
    float4 Q1   = fetch(Q_in,x+1,y,1);
    float4 Sxp  = fetch(Sx_in,x+1,y,1);
    float4 Syp  = fetch(Sy_in,x+1,y,1);
    
    if(y == 0){
        store(F_out, (float4)(0.0f,0.0f,0.0f,0.0f), x, y, 1);
    }else{
        store(F_out, xFlux(k, g, Q, Q1, Sx, Sy, Sxp, Syp), x, y, 1);
    }
    //store(F_out, (float)x+1, x, y, 2);
    
    Q1   = fetch(Q_in,x,y+1,1);
    Sxp  = fetch(Sx_in,x,y+1,1);
    Syp  = fetch(Sy_in,x,y+1,1);
    
    if(x == 0){
        store(G_out, (float4)(0.0f,0.0f,0.0f,0.0f), x, y, 1);
    }else{
        store(G_out, yFlux(k, g, Q, Q1, Sx, Sy, Sxp, Syp), x, y, 1);
    }
    //store(G_out, (float)y+1, x, y, 2);
}


/****
 *
 * Compute eigenvalues
 *
 ****/
__kernel void eigenvalue(__global float4* Q_in, float g, __global float* E_out){
    unsigned int x = get_global_id(0);
    unsigned int y = get_global_id(1);
    
    float4 Q    = fetch(Q_in, x, y,2);
    if(Q.x <= 1.19e-07f){
        storef(E_out, 0.0f, x, y,2);
        return;
    }
    
    float2 uv   = Q.yz/Q.x;
    float c     = sqrt(g*Q.x);
    
    float eigen;
    eigen = max(fabs(uv.x)-c,0.0f);
    eigen = max(fabs(uv.x)+c,fabs(eigen));
    eigen = max(fabs(uv.y)-c,fabs(eigen));
    eigen = max(fabs(uv.y)+c,fabs(eigen));
    
    storef(E_out, eigen, x, y,2);
}
