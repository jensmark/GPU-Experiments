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
	 * Simulation step
	 */
    void advect(double dt);
    
    /**
	 * Simulation step
	 */
    void diffuse(double dt);
    
    /**
	 * Simulation step
	 */
    void addForce(double dt);
    
    /**
	 * Simulation step
	 */
    void compPressure(double dt);
    
    /**
	 * Simulation step
	 */
    void subPressureGrad(double dt);
    
    /**
	 * Swap FBO pointers
	 */
    void swap(TextureFBO* a, TextureFBO* b);
    
    /**
	 * Function that runs sim kernel
	 */
    void runSimulation(double dt);
    
	/**
	 * Function that handles rendering into the OpenGL context
	 */
	void render();
    
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
    
    static const unsigned int Nx            = 512;
    static const unsigned int Ny            = 512;
    
    static const unsigned int window_width  = 800;
	static const unsigned int window_height = 600;
    
    // GLFW window handle
    GLFWwindow* window;
    
    // Timer
    Timer timer;
    
    //Program* advect;
    //Program* diffuse;
    Program* add_force;
    Program* comp_pressure;
    Program* sub_pressure_grad;
    Program* visualize;
    Program* copy;
    
    BO<GL_ARRAY_BUFFER>* vert;
    BO<GL_ELEMENT_ARRAY_BUFFER>* ind;
    
    TextureFBO* kernel0;
    TextureFBO* kernel1;
    
    GLuint vao;
};

#endif
