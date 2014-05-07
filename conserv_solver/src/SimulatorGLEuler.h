//
//  SimulatorCLSW
//  GLAppNative
//
//  Created by Jens Kristoffer Reitan Markussen on 28.12.13.
//  Copyright (c) 2013 Jens Kristoffer Reitan Markussen. All rights reserved.
//

#ifndef GLAppNative_SimulatorGLEuler_h
#define GLAppNative_SimulatorGLEuler_h

#include "GLUtils.hpp"
#include "SimulatorBase.h"
#include "Timer.hpp"

#include "glm/glm.hpp"
#include "TextureFBO.h"

#include <vector>


class SimulatorGLEuler : public SimulatorBase{
public:
    /**
	 * Constructor
	 */
	SimulatorGLEuler();
    
	/**
	 * Destructor
	 */
	virtual ~SimulatorGLEuler();
    
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
	 * Compiles, attaches, links, and sets uniforms for
	 * a simple OpenGL program
	 */
	void createProgram(std::string initial);
    
	/**
	 * Creates vertex array objects
	 */
	void createVAO();
    
	/**
     * Sets up the FBO for us
     */
	void createFBO();

    
    /**
	 * Function that applies initial simulation state
	 */
    void applyInitial();
    
    /**
     * Function that enforces boundary condition
     */
    void setBoundary(TextureFBO* Qn);
    
    /**
     * Computes timestep based on CFL
     */
    float computeDt(TextureFBO* Qn);
    
    /**
	 * Simulation step
	 */
    void reconstruct(TextureFBO* Qn);
    
    /**
	 * Simulation step
	 */
    void evaluateFluxes(TextureFBO* Qn);
    
    /**
	 * Simulation step
	 */
    void computeRK(size_t n, float dt);
    
    /**
	 * Copy texture to framebuffer texture
	 */
    void copyTexture(GLint source, TextureFBO* dest);
    
private:
    
    size_t Nx;
    size_t Ny;
    
    static const unsigned int N_RK  = 3;
    float gamma;
    float time;
    
    // Timer
    Timer   timer;
    
    GLUtils::Program* runge_kutta;
    GLUtils::Program* bilinear_recon;
    GLUtils::Program* flux_evaluator;
    GLUtils::Program* copy;
    GLUtils::Program* eigen;
    GLUtils::Program* initialK;
    
    GLUtils::BO<GL_ARRAY_BUFFER>* vert;
    GLUtils::BO<GL_ELEMENT_ARRAY_BUFFER>* ind;
    
    TextureFBO* kernelRK[N_RK+1];
    TextureFBO* reconstructKernel;
    TextureFBO* fluxKernel;
    TextureFBO* dtKernel;
    
    GLuint vao[2];

};

#endif
