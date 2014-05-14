/***
 * Function dec
 ****/
float4  dambreakAt(float2 pos);
float4  shockbubbleAt(float gamma, float2 pos);
float4  riemannAt(float gamma, float2 pos, float4 R1, float4 R2, float4 R3, float4 R4);

float E(float rho, float u, float v, float gamma, float p);

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
 * Dambreak
 *
 ****/
float4 dambreakAt(float2 pos){
    float4 value = (float4)(1.0f,0.0f,0.0f,0.0f);
    
    if(pos.x < 0.5f){
        value.x = 1.5f;
    }
    
    return value;
}

__kernel void dambreak(float g, float2 dXY, __global float4* Q_out){
    unsigned int x = get_global_id(0);
    unsigned int y = get_global_id(1);
    
    float xfac  = 0.28867513459481288225f*dXY.x;
    float yfac  = 0.28867513459481288225f*dXY.y;
    
    float2 pos  = (float2)((float)x/(float)Nx,(float)y/(float)Ny);
    float2 pos0 = (float2)(pos.x-xfac, pos.y-yfac);
    float2 pos1 = (float2)(pos.x+xfac, pos.y-yfac);
    float2 pos2 = (float2)(pos.x-xfac, pos.y+yfac);
    float2 pos3 = (float2)(pos.x+xfac, pos.y+yfac);
    
    float4 value0 = dambreakAt(pos0);
    float4 value1 = dambreakAt(pos1);
    float4 value2 = dambreakAt(pos2);
    float4 value3 = dambreakAt(pos3);

    store(Q_out,(value0+value1+value2+value3)*0.25f,x,y,2);
}


/****
 *
 * Shock-bubble
 *
 ****/
float E(float rho, float u, float v, float gamma, float p){
    return 0.5f*rho*(u*u+v*v)+p/(gamma-1.0f);
}

float4 shockbubbleAt(float gamma, float2 pos){
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

__kernel void shockbubble(float gamma, float2 dXY, __global float4* Q_out){
    unsigned int x = get_global_id(0);
    unsigned int y = get_global_id(1);
    
    float xfac  = 0.28867513459481288225f*dXY.x;
    float yfac  = 0.28867513459481288225f*dXY.y;
    
    float2 pos  = (float2)((float)x/(float)Nx,(float)y/(float)Ny);
    float2 pos0 = (float2)(pos.x-xfac, pos.y-yfac);
    float2 pos1 = (float2)(pos.x+xfac, pos.y-yfac);
    float2 pos2 = (float2)(pos.x-xfac, pos.y+yfac);
    float2 pos3 = (float2)(pos.x+xfac, pos.y+yfac);
    
    float4 value0 = shockbubbleAt(gamma, pos0);
    float4 value1 = shockbubbleAt(gamma, pos1);
    float4 value2 = shockbubbleAt(gamma, pos2);
    float4 value3 = shockbubbleAt(gamma, pos3);
    
    store(Q_out,(value0+value1+value2+value3)*0.25f,x,y,2);
}

float4  riemannAt(float gamma, float2 pos, float4 R1, float4 R2, float4 R3, float4 R4){
    float4 value = (float4)(0.0f,0.0f,0.0f,0.0f);
    
    if (pos.x >= 0.5f && pos.y >= 0.5f) {
        value = R1;
    }
    else if (pos.x <= 0.5f && pos.y >= 0.5f){
        value = R2;
    }
    else if (pos.x <= 0.5f && pos.y <= 0.5f){
        value = R3;
    }
    else if (pos.x >= 0.5f && pos.y <= 0.5f){
        value = R4;
    }
    
    return value;
}

__kernel void riemann(float gamma, float2 dXY, __global float4* Q_out){
    unsigned int x = get_global_id(0);
    unsigned int y = get_global_id(1);
    
    float xfac  = 0.28867513459481288225f*dXY.x;
    float yfac  = 0.28867513459481288225f*dXY.y;
    
    float4 R1 = (float4)(1.5f,0.0f,0.0f,E(1.5f, 0.0f, 0.0f, gamma, 1.5f));
    float4 R2 = (float4)(0.5323f,0.5323f*1.206f,0.0f,E(0.5323f, 1.206f, 0.0f, gamma, 0.3f));
    float4 R3 = (float4)(0.138f,0.138f*1.206f,0.138f*1.206f,E(0.138f, 1.206f, 1.206f, gamma, 0.028f));
    float4 R4 = (float4)(0.5323f,0.0f,0.5323f*1.206f,E(0.5323f, 0.0f, 1.206f, gamma, 0.3f));
    
    float2 pos  = (float2)((float)x/(float)Nx,(float)y/(float)Ny);
    float2 pos0 = (float2)(pos.x-xfac, pos.y-yfac);
    float2 pos1 = (float2)(pos.x+xfac, pos.y-yfac);
    float2 pos2 = (float2)(pos.x-xfac, pos.y+yfac);
    float2 pos3 = (float2)(pos.x+xfac, pos.y+yfac);
    
    float4 value0 = riemannAt(gamma, pos0, R1, R2, R3, R4);
    float4 value1 = riemannAt(gamma, pos1, R1, R2, R3, R4);
    float4 value2 = riemannAt(gamma, pos2, R1, R2, R3, R4);
    float4 value3 = riemannAt(gamma, pos3, R1, R2, R3, R4);
    
    store(Q_out,(value0+value1+value2+value3)*0.25f,x,y,2);
}
