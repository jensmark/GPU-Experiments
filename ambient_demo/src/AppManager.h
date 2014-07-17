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
#include "Model.h"
#include "TextureFBO.h"

#include <IL/IL.h>
#include <IL/ILU.h>

#include <vector>

using namespace GLUtils;

enum RenderMode{
    PHONG
};

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
	void init(std::string model);
    
	/**
	 * The main loop of the app. Runs the main loop
	 */
	void begin();
    
private:
    /**
	 * Function that handles rendering into the OpenGL context
	 */
	void render();
    
    void renderPhong();
    
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
     * Callback for GLFW mouse scrollwheel
     */
    static void scrollwheel_callback(GLFWwindow* window, double xoffset, double yoffset);
    
    /**
     * Callback for GLFW error
     */
    static void error_callback(int error, const char* description) {
        THROW_EXCEPTION(description);
    }
    
private:
    static void renderMeshRecursive(MeshPart& mesh, const std::shared_ptr<GLUtils::Program>& program, const glm::mat4& modelview, const glm::mat4& transform);
    
    template<class T>
    T random(T min, T max){
        return min + static_cast <T> (rand()) /( static_cast <T> (RAND_MAX/(max-min)));
    }
    
private:
    // GLFW window handle
    GLFWwindow* window;
    
    std::shared_ptr<TextureFBO> deferred_kernel;
    std::shared_ptr<TextureFBO> ao_kernel;
    std::shared_ptr<TextureFBO> blur_kernel;
    
    size_t sample_count;
    std::vector<float> samples;
    
    size_t noise_size;
    GLuint noiseTexture;
    
    static const unsigned int window_width  = 800;
	static const unsigned int window_height = 600;
    
    struct {
		glm::mat4 projection;
		glm::mat4 view;
	} camera;
    
    static VirtualTrackball trackball;
    
    std::string filepath;
    
    GLuint vao[2];
    std::shared_ptr<Model> model;
    glm::mat4 model_matrix;
    
    std::shared_ptr<GLUtils::BO<GL_ARRAY_BUFFER>> vert;
    std::shared_ptr<GLUtils::BO<GL_ELEMENT_ARRAY_BUFFER>> ind;
    
    std::shared_ptr<GLUtils::Program> deferred;
    std::shared_ptr<GLUtils::Program> phong;
    std::shared_ptr<GLUtils::Program> ssao;
    // Program used to debug render textures
    std::shared_ptr<GLUtils::Program> debug;
    
    static RenderMode mode;
    static float fovx;
};

#endif
