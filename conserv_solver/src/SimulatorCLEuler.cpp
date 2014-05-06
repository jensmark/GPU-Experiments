//
//  SimulatorCLSW
//  GLAppNative
//
//  Created by Jens Kristoffer Reitan Markussen on 28.12.13.
//  Copyright (c) 2013 Jens Kristoffer Reitan Markussen. All rights reserved.
//

#include "SimulatorCLEuler.h"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>

SimulatorCLEuler::SimulatorCLEuler(cl_device_type device){
    this->gamma = 1.4f;
    this->time = 0;
    
    CLUtils::createContext(context,device);
}

SimulatorCLEuler::~SimulatorCLEuler(){
    clReleaseKernel(compute_reconstruct);
    clReleaseKernel(evaluate_flux);
    clReleaseKernel(compute_RK);
    clReleaseKernel(compute_eigenvalues);
    clReleaseKernel(copy_domain);
    clReleaseKernel(prepare_render);
    clReleaseKernel(set_initial);
    clReleaseKernel(set_boundary_x);
    clReleaseKernel(set_boundary_y);
    
    delete Sx_set;
    delete Sy_set;
    delete F_set;
    delete G_set;
    delete E_set;
    delete R_tex;
    for (size_t i = 0; i <= N_RK; i++) {
        delete Q_set[i];
    }

    CLUtils::releaseContext(context);
}

void SimulatorCLEuler::init(size_t Nx, size_t Ny, std::string initialKernel){
    this->Nx    = Nx;
    this->Ny    = Ny;
    
    std::cout << "Simulating Euler using OpenCL kernels on device: ";
    CLUtils::printDeviceInfo(context.device);
    
    createKernels("shockbubble");
    createBuffers();
    
    applyInitial();
}

SimDetail SimulatorCLEuler::simulate(){
    setBoundary(Q_set[N_RK]);
    copy(Q_set[N_RK], Q_set[0]);

    float dt = computeDt(Q_set[0]);
    
    timer.restart();
    SimDetail detail;
    detail.sim_time = 0.0f;
    
    for (size_t n = 1; n <= N_RK; n++) {
        // apply boundary condition
        setBoundary(Q_set[n-1]);
        
        timer.restart();
        
        // reconstruct point values
        reconstruct(Q_set[n-1]);
        clFinish(context.queue);
        
        detail.sim_time += timer.elapsedAndRestart();
        
        // evaluate fluxes
        evaluateFluxes(Q_set[n-1]);
        clFinish(context.queue);
        
        detail.sim_time += timer.elapsedAndRestart();
        
        // compute RK
        computeRK(n, dt);
        clFinish(context.queue);
        
        detail.sim_time += timer.elapsed();
    }
    
    clFinish(context.queue);
    
    //detail.sim_time = timer.elapsed();
    detail.dt = dt;
    time+=dt;
    detail.time = time;

    
    return detail;
}

size_t SimulatorCLEuler::getTexture(){
    cl_int err = CL_SUCCESS;
    
    err |= clSetKernelArg(prepare_render, 0, sizeof(cl_mem), &Q_set[N_RK]->getRef());
    err |= clSetKernelArg(prepare_render, 1, sizeof(cl_image), &(R_tex->getRef()));
    
    size_t global[] = {Nx,Ny};
    err |= clEnqueueNDRangeKernel(context.queue, prepare_render, 2, NULL, global, NULL, 0, NULL, NULL);
    if(err != CL_SUCCESS) {
        std::stringstream ss;
        ss << "Failed to prepare render! Error: " << err;
        THROW_EXCEPTION(ss.str().c_str());
    }
    
    return tex;
}

std::vector<float> SimulatorCLEuler::getData(){
    THROW_EXCEPTION("Not implemented");
}

void SimulatorCLEuler::createBuffers(){
    // Initialize all buffers with 2 ghost cell on each edge
    for (size_t i = 0; i <= N_RK; i++) {
        Q_set[i] = new CLUtils::MO<CL_MEM_READ_WRITE>(context, (Nx+4)*(Ny+4)*sizeof(cl_float4), NULL);
    }
    Sx_set = new CLUtils::MO<CL_MEM_READ_WRITE>(context, (Nx+4)*(Ny+4)*sizeof(cl_float4), NULL);
    Sy_set = new CLUtils::MO<CL_MEM_READ_WRITE>(context, (Nx+4)*(Ny+4)*sizeof(cl_float4), NULL);
    F_set  = new CLUtils::MO<CL_MEM_READ_WRITE>(context, (Nx+4)*(Ny+4)*sizeof(cl_float4), NULL);
    G_set  = new CLUtils::MO<CL_MEM_READ_WRITE>(context, (Nx+4)*(Ny+4)*sizeof(cl_float4), NULL);
    E_set  = new CLUtils::MO<CL_MEM_WRITE_ONLY>(context, (Nx+4)*(Ny+4)*sizeof(cl_float), NULL);
    
    // We dont need to visualize ghost cells
    R_tex  = new CLUtils::ImageBuffer<CL_MEM_READ_WRITE>(context, tex, (Nx), (Ny), NULL);
}

void SimulatorCLEuler::createKernels(std::string initial){
    std::stringstream ss;
    ss << "-D Nx=" << Nx << " -D Ny=" << Ny;
    std::string options = ss.str();
    
    CLUtils::Program* common    = new CLUtils::Program(context, "res/kernels/common.cl", &options);
    CLUtils::Program* boundary  = new CLUtils::Program(context, "res/kernels/boundary.cl", &options);
    CLUtils::Program* initialp  = new CLUtils::Program(context, "res/kernels/initial.cl", &options);
    CLUtils::Program* euler        = new CLUtils::Program(context, "res/kernels/euler.cl", &options);
    
    
    compute_reconstruct = common->createKernel("piecewiseReconstruction");
    evaluate_flux       = euler->createKernel("computeNumericalFlux");
    compute_RK          = common->createKernel("computeRK");
    compute_eigenvalues = euler->createKernel("eigenvalue");
    copy_domain         = common->createKernel("copy");
    prepare_render      = common->createKernel("copyToTexture");
    set_initial         = initialp->createKernel(initial);
    set_boundary_x      = boundary->createKernel("setBoundsX");
    set_boundary_y      = boundary->createKernel("setBoundsY");
}

void SimulatorCLEuler::applyInitial(){
    cl_int err = CL_SUCCESS;
    
    err |= clSetKernelArg(set_initial, 0, sizeof(cl_float), &gamma);
    err |= clSetKernelArg(set_initial, 1, sizeof(cl_float2),
                          glm::value_ptr(glm::vec2((1.0f/(float)Nx),(1.0f/(float)Ny))));
    err |= clSetKernelArg(set_initial, 2, sizeof(cl_mem), &(Q_set[N_RK]->getRef()));
    
    size_t global[] = {Nx,Ny};
    err |= clEnqueueNDRangeKernel(context.queue, set_initial, 2, NULL, global, NULL, 0, NULL, NULL);
    
    if(err != CL_SUCCESS) {
        std::stringstream ss;
        ss << "Failed to apply initial condition! Error: " << err;
        THROW_EXCEPTION(ss.str().c_str());
    }
}

void SimulatorCLEuler::setBoundary(CLUtils::MO<CL_MEM_READ_WRITE>* Qn){
    cl_int err = CL_SUCCESS;
    
    err |= clSetKernelArg(set_boundary_x, 0, sizeof(cl_mem), &(Qn->getRef()));
    size_t globalx[] = {Nx};
    err |= clEnqueueNDRangeKernel(context.queue, set_boundary_x,
                                  1, NULL, globalx, NULL, 0, NULL, NULL);
    
    err |= clSetKernelArg(set_boundary_y, 0, sizeof(cl_mem), &(Qn->getRef()));
    size_t globaly[] = {Ny};
    err |= clEnqueueNDRangeKernel(context.queue, set_boundary_y,
                                  1, NULL, globaly, NULL, 0, NULL, NULL);
    
    if(err != CL_SUCCESS) {
        std::stringstream ss;
        ss << "Failed to set boundary! Error: " << err;
        THROW_EXCEPTION(ss.str().c_str());
    }
}

float SimulatorCLEuler::computeDt(CLUtils::MO<CL_MEM_READ_WRITE>* Qn){
    cl_int err = CL_SUCCESS;
    static const float CFL = 0.5f;
    
    err |= clSetKernelArg(compute_eigenvalues, 0, sizeof(cl_mem), &(Qn->getRef()));
    err |= clSetKernelArg(compute_eigenvalues, 1, sizeof(cl_float), &gamma);
    err |= clSetKernelArg(compute_eigenvalues, 2, sizeof(cl_mem), &(E_set->getRef()));
    
    size_t global[] = {Nx,Ny};
    err |= clEnqueueNDRangeKernel(context.queue, compute_eigenvalues,
                                  2, NULL, global, NULL, 0, NULL, NULL);
    
    std::vector<cl_float> data((Nx+4)*(Ny+4));
    err |= clEnqueueReadBuffer(context.queue, E_set->getRef(), CL_TRUE, 0,
                               (Nx+4)*(Ny+4)*sizeof(cl_float), data.data(), 0, NULL, NULL);
    
    if(err != CL_SUCCESS) {
        std::stringstream ss;
        ss << "Failed to compute dt! Error: " << err;
        THROW_EXCEPTION(ss.str().c_str());
    }
    
    float eig = -std::numeric_limits<float>().max();
    
    for (size_t x = 2; x < Nx+2; x++) {
        for (size_t y = 2; y < Ny+2; y++) {
            size_t k = ((Nx+4) * y + x);
            eig = glm::max(eig, data[k]);
        }
    }
    
    float dx = 1.0f/(float)Nx;
    float dy = 1.0f/(float)Ny;
    float dt = CFL*glm::min(dx/eig,dy/eig);
    
    //std::cout << "Dt: " << dt << " Max eigenvalue: " << eig << std::endl << std::endl;
    
    return dt;
}

void SimulatorCLEuler::reconstruct(CLUtils::MO<CL_MEM_READ_WRITE>* Qn){
    cl_int err = CL_SUCCESS;
    
    err |= clSetKernelArg(compute_reconstruct, 0, sizeof(cl_mem), &(Qn->getRef()));
    err |= clSetKernelArg(compute_reconstruct, 1, sizeof(cl_mem), &(Sx_set->getRef()));
    err |= clSetKernelArg(compute_reconstruct, 2, sizeof(cl_mem), &(Sy_set->getRef()));
    
    size_t global[] = {Nx+2,Ny+2};
    err |= clEnqueueNDRangeKernel(context.queue, compute_reconstruct, 2,
                                  NULL, global, NULL, 0, NULL, NULL);
    
    if(err != CL_SUCCESS) {
        std::stringstream ss;
        ss << "Failed to run reconstruction! Error: " << err;
        THROW_EXCEPTION(ss.str().c_str());
    }
}

void SimulatorCLEuler::evaluateFluxes(CLUtils::MO<CL_MEM_READ_WRITE>* Qn){
    cl_int err = CL_SUCCESS;
    
    err |= clSetKernelArg(evaluate_flux, 0, sizeof(cl_mem), &(Qn->getRef()));
    err |= clSetKernelArg(evaluate_flux, 1, sizeof(cl_mem), &(Sx_set->getRef()));
    err |= clSetKernelArg(evaluate_flux, 2, sizeof(cl_mem), &(Sy_set->getRef()));
    err |= clSetKernelArg(evaluate_flux, 3, sizeof(cl_float), &gamma);
    err |= clSetKernelArg(evaluate_flux, 4, sizeof(cl_mem), &(F_set->getRef()));
    err |= clSetKernelArg(evaluate_flux, 5, sizeof(cl_mem), &(G_set->getRef()));
    
    size_t global[] = {Nx+1,Ny+1};
    err |= clEnqueueNDRangeKernel(context.queue, evaluate_flux, 2,
                                  NULL, global, NULL, 0, NULL, NULL);
    
    if(err != CL_SUCCESS) {
        std::stringstream ss;
        ss << "Failed to evalute fluxes! Error: " << err;
        THROW_EXCEPTION(ss.str().c_str());
    }
}

void SimulatorCLEuler::computeRK(size_t n, float dt){
    cl_int err = CL_SUCCESS;
    
    static const glm::vec2 c[3][3] =
    {
        {glm::vec2(0.0f,1.0f),
            glm::vec2(0.0f,0.0f),
            glm::vec2(0.0f,0.0f)},
        
        {glm::vec2(0.0f,1.0f),
            glm::vec2(0.5f,0.5f),
            glm::vec2(0.0f,0.0f)},
        
        {glm::vec2(0.0f,1.0f),
            glm::vec2(0.75f,0.25f),
            glm::vec2(0.333f,0.666f)}
    };
    
    err |= clSetKernelArg(compute_RK, 0, sizeof(cl_mem), &(Q_set[0]->getRef()));
    err |= clSetKernelArg(compute_RK, 1, sizeof(cl_mem), &(Q_set[n-1]->getRef()));
    err |= clSetKernelArg(compute_RK, 2, sizeof(cl_mem), &(F_set->getRef()));
    err |= clSetKernelArg(compute_RK, 3, sizeof(cl_mem), &(G_set->getRef()));
    err |= clSetKernelArg(compute_RK, 4, sizeof(cl_float2), glm::value_ptr(c[N_RK-1][n-1]));
    err |= clSetKernelArg(compute_RK, 5, sizeof(cl_float2),
                          glm::value_ptr(glm::vec2((1.0f/(float)Nx),(1.0f/(float)Ny))));
    err |= clSetKernelArg(compute_RK, 6, sizeof(cl_float), &dt);
    err |= clSetKernelArg(compute_RK, 7, sizeof(cl_mem), &(Q_set[n]->getRef()));
    
    size_t global[] = {Nx,Ny};
    err |= clEnqueueNDRangeKernel(context.queue, compute_RK, 2,
                                  NULL, global, NULL, 0, NULL, NULL);
    
    if(err != CL_SUCCESS) {
        std::stringstream ss;
        ss << "Failed to run rk kernel! Error: " << err;
        THROW_EXCEPTION(ss.str().c_str());
    }
}

void SimulatorCLEuler::copy(CLUtils::MO<CL_MEM_READ_WRITE>* src,
                         CLUtils::MO<CL_MEM_READ_WRITE>* dest){
    cl_int err = CL_SUCCESS;
    
    err |= clSetKernelArg(copy_domain, 0, sizeof(cl_mem), &(src->getRef()));
    err |= clSetKernelArg(copy_domain, 1, sizeof(cl_mem), &(dest->getRef()));
    
    size_t global[] = {Nx+4,Ny+4};
    err |= clEnqueueNDRangeKernel(context.queue, copy_domain, 2, NULL, global, NULL, 0, NULL, NULL);
    
    if(err != CL_SUCCESS) {
        std::stringstream ss;
        ss << "Failed to copy domain! Error: " << err;
        THROW_EXCEPTION(ss.str().c_str());
    }
}
