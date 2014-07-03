//
//  AppManager.h
//  GLAppNative
//
//  Created by Jens Kristoffer Reitan Markussen on 28.12.13.
//  Copyright (c) 2013 Jens Kristoffer Reitan Markussen. All rights reserved.
//

#ifndef GLAppNative_AppManager_h
#define GLAppNative_AppManager_h

#include "GLUtils.hpp"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

#include "VirtualTrackball.h"
#include <IL/IL.h>
#include <IL/ILU.h>

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
	 * Function that handles rendering into the OpenGL context
	 */
	void render();
    
    /**
	 * Quit function
	 */
	void quit();
    
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
    
    static const unsigned int window_width  = 800;
	static const unsigned int window_height = 600;
    
    struct {
		glm::mat4 projection;
		glm::mat4 view;
	} camera;
    
    static VirtualTrackball trackball;
    
};

#endif
