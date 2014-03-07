//
//  AppManager.h
//  GLAppNative
//
//  Created by Jens Kristoffer Reitan Markussen on 28.12.13.
//  Copyright (c) 2013 Jens Kristoffer Reitan Markussen. All rights reserved.
//

#ifndef GLAppNative_AppManager_h
#define GLAppNative_AppManager_h

#include "GLUtils/GLUtils.hpp"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

#include "Timer.hpp"
#include "TextureFBO.h"

#include <vector>

using namespace GLUtils;

class AppManager{
public:
    /**
	 * Constructor
	 */
	AppManager();
    
	/**
	 * Destructor
	 */
	~AppManager();
    
	/**
	 * Initializes the game, including the OpenGL context
	 * and data required
	 */
	void init();
    
	/**
	 * The main loop of the app. Runs the main loop
	 */
	void begin();
    
private:
    /**
	 * Quit function
	 */
	void quit();
    
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
    
    /**
	 * Function that runs sim kernel
	 */
    void runKernel();
    
	/**
	 * Function that handles rendering into the OpenGL context
	 */
	void render();
    
    /**
	 * Downloads latest simulation result for debugging
	 */
    void debugDownload(bool texDump);
    
	/**
	 * Creates the OpenGL context using GLFW
	 */
	void createOpenGLContext();
    
	/**
	 * Sets states for OpenGL that we want to keep persistent
	 * throughout the game
	 */
	void setOpenGLStates();
    
	/**
	 * Compiles, attaches, links, and sets uniforms for
	 * a simple OpenGL program
	 */
	void createProgram();
    
	/**
	 * Creates vertex array objects
	 */
	void createVAO();
    
	/**
     * Sets up the FBO for us
     */
	void createFBO();
    
    /**
     * Callback for GLFW key press
     */
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    
    /**
     * Callback for GLFW mouse button press
     */
    static void mouse_callback(GLFWwindow* window, int button, int action, int mods);
    
    /**
     * Callback for GLFW cursor motion
     */
    static void cursor_callback(GLFWwindow* window, double x, double y);
    
    /**
     * Callback for GLFW error
     */
    static void error_callback(int error, const char* description) {
        THROW_EXCEPTION(description);
    }
    
private:
    
    static const unsigned int N_RK          = 3;
    
    static const unsigned int Nx            = 512;
    static const unsigned int Ny            = 512;
    
    static const unsigned int window_width  = 800;
	static const unsigned int window_height = 600;
    
    float gamma;
    float pressure;
    
    // GLFW window handle
    GLFWwindow* window;
    
    // Timer
    Timer   timer;
    float   time;
    size_t  step;
    
    Program* visualize;
    Program* runge_kutta;
    Program* bilinear_recon;
    Program* flux_evaluator;
    Program* copy;
    Program* boundary;
    Program* eigen;
    Program* gradient;
    
    BO<GL_ARRAY_BUFFER>* vert;
    BO<GL_ELEMENT_ARRAY_BUFFER>* ind;
    BO<GL_ARRAY_BUFFER>* b_vert;
    
    TextureFBO* kernelRK[N_RK+1];
    TextureFBO* reconstructKernel;
    TextureFBO* fluxKernel;
    TextureFBO* dtKernel;
    TextureFBO* gradKernel;
    
    GLuint vao[2];
};

#endif
