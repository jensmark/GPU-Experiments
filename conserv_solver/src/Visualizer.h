//
//  Visualizer
//  GLAppNative
//
//  Created by Jens Kristoffer Reitan Markussen on 28.12.13.
//  Copyright (c) 2013 Jens Kristoffer Reitan Markussen. All rights reserved.
//

#ifndef GLAppNative_Visualizer_h
#define GLAppNative_Visualizer_h


#include "GLUtils.hpp"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "VirtualTrackball.h"

class Visualizer{
public:
    /**
	 * Constructor
	 */
	Visualizer(unsigned int width,unsigned int height);
    
	/**
	 * Destructor
	 */
	virtual ~Visualizer();
    
	/**
	 * Initializes the visualizer
	 */
	void init();
    
    /**
	 * Function that handles rendering into the OpenGL context
	 */
	void render(GLuint tex);
    
    /**
     * Return window handle
     */
    GLFWwindow* getWindow(){return window;}
    
private:
    
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
     * Creates a NxNy size triangle strip grid
     */
    void createTriangleStrip(unsigned int Nx, unsigned int Ny, GLUtils::BO<GL_ARRAY_BUFFER>*& vert, GLUtils::BO<GL_ELEMENT_ARRAY_BUFFER>*& ind);

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
    // GLFW window handle
    GLFWwindow* window;
    
    unsigned int width;
    unsigned int height;
    
    //GLuint tex;
    GLUtils::Program* visualize;
    
    GLUtils::BO<GL_ARRAY_BUFFER>* surface_vert;
    GLUtils::BO<GL_ELEMENT_ARRAY_BUFFER>* surface_ind;
    
    unsigned int restart_token;
    unsigned int indices_count;

    GLuint vao;
    
    glm::mat4 model;
    struct {
		glm::mat4 projection;
		glm::mat4 view;
	} camera;
    
    static VirtualTrackball trackball;
};

#endif
