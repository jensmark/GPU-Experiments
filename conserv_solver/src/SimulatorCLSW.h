//
//  SimulatorCLSW
//  GLAppNative
//
//  Created by Jens Kristoffer Reitan Markussen on 28.12.13.
//  Copyright (c) 2013 Jens Kristoffer Reitan Markussen. All rights reserved.
//

#ifndef GLAppNative_SimulatorCLSW_h
#define GLAppNative_SimulatorCLSW_h

#include "GLUtils.hpp"
#include "CLUtils.hpp"
#include "SimulatorBase.h"
#include "Timer.hpp"

class SimulatorCLSW : public SimulatorBase{
public:
    /**
	 * Constructor
	 */
	SimulatorCLSW(cl_device_type device);
    
	/**
	 * Destructor
	 */
	virtual ~SimulatorCLSW();
    
	/**
	 * Initializes the simulator
	 */
	virtual void init(size_t Nx, size_t Ny, std::string initialKernel);
    
    /**
     * Run one step of the simulator
     */
    virtual SimDetail simulate();
    
    /**
     * Get the data as opengl texture
     */
    virtual size_t getTexture();
    
    /**
     * Get the data as std vector
     */
    virtual std::vector<float> getData();
    
    /**
     * Return the size of the grid
     */
    virtual glm::ivec2 getGridSize(){return glm::ivec2(Nx,Ny);}
    
    /**
     * Return grid delta x and y
     */
    virtual glm::vec2 getDeltaXY(){return glm::vec2(1.0f/(float)Nx,1.0f/(float)Ny);}
    
    /**
     * Returns time
     */
    virtual float getTime(){return time;}
    
private:
    /**
     * Sets up the buffers for us
     */
	void createBuffers();
    
    /**
	 * Create and compile OpenCL kernels
	 */
	void createKernels(std::string initial);
    
    /**
	 * Function that applies initial simulation state
	 */
    void applyInitial();
    
    /**
     * Function that enforces boundary condition
     */
    void setBoundary(CLUtils::MO<CL_MEM_READ_WRITE>* Qn);
    
    /**
     * Computes timestep based on CFL
     */
    float computeDt(CLUtils::MO<CL_MEM_READ_WRITE>* Qn);
    
    /**
	 * Simulation step
	 */
    void reconstruct(CLUtils::MO<CL_MEM_READ_WRITE>* Qn);
    
    /**
	 * Simulation step
	 */
    void evaluateFluxes(CLUtils::MO<CL_MEM_READ_WRITE>* Qn);
    
    /**
	 * Simulation step
	 */
    void computeRK(size_t n, float dt);
    
    /**
	 * Copy texture to framebuffer texture
	 */
    void copy(CLUtils::MO<CL_MEM_READ_WRITE>* src,
              CLUtils::MO<CL_MEM_READ_WRITE>* dest);
    
private:
    CLUtils::CLcontext context;
    
    size_t Nx;
    size_t Ny;
    
    static const unsigned int N_RK  = 3;
    float gravity;
    float time;
    
    GLuint tex;
    
    cl_kernel           compute_reconstruct;
    cl_kernel           evaluate_flux;
    cl_kernel           compute_RK;
    cl_kernel           compute_eigenvalues;
    cl_kernel           copy_domain;
    cl_kernel           prepare_render;
    cl_kernel           set_initial;
    cl_kernel           set_boundary_x;
    cl_kernel           set_boundary_y;
    
    CLUtils::MO<CL_MEM_READ_WRITE>*             Q_set[N_RK+1];
    CLUtils::MO<CL_MEM_READ_WRITE>*             Sx_set;
    CLUtils::MO<CL_MEM_READ_WRITE>*             Sy_set;
    CLUtils::MO<CL_MEM_READ_WRITE>*             F_set;
    CLUtils::MO<CL_MEM_READ_WRITE>*             G_set;
    CLUtils::MO<CL_MEM_WRITE_ONLY>*             E_set;
    
    CLUtils::ImageBuffer<CL_MEM_READ_WRITE>*    R_tex;
    
    Timer timer;
};

#endif
