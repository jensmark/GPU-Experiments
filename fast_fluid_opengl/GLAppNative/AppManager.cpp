//
//  AppManager.cpp
//  GLAppNative
//
//  Created by Jens Kristoffer Reitan Markussen on 28.12.13.
//  Copyright (c) 2013 Jens Kristoffer Reitan Markussen. All rights reserved.
//

#include "AppManager.h"
#include <vector>
#include <glm/glm.hpp>

AppManager::AppManager():
    visualize(NULL),
    copy(NULL),
    vert(NULL),
    ind(NULL),
    kernel0(NULL),
    kernel1(NULL){
}

AppManager::~AppManager(){
}

void AppManager::init(){
    /* Initialize the library */
    if (glfwInit() != GL_TRUE) {
        THROW_EXCEPTION("Failed to initialize GLFW");
    }
    glfwSetErrorCallback(error_callback);
    
    createOpenGLContext();
    setOpenGLStates();
    createFBO();
    createProgram();
    createVAO();
}

void AppManager::begin(){
    while (!glfwWindowShouldClose(window)) {
        /* Poll for and process events */
        glfwPollEvents();
        
        /* Render loop */
        render();
        
        /* Swap front and back buffers */
        glfwSwapBuffers(window);
    }
    
    /* Clean up everything */
    quit();
}

void AppManager::quit(){
    delete visualize;
    delete copy;
    delete vert;
    delete ind;
    delete kernel0;
    delete kernel1;
    
    glfwDestroyWindow(window);
    glfwTerminate();
}

void AppManager::advect(double dt){
    CHECK_GL_ERRORS();
}

void AppManager::diffuse(double dt){
    CHECK_GL_ERRORS();
}

void AppManager::addForce(double dt){
    CHECK_GL_ERRORS();
}

void AppManager::compPressure(double dt){
    CHECK_GL_ERRORS();
}

void AppManager::subPressureGrad(double dt){
    CHECK_GL_ERRORS();
}

void AppManager::swap(TextureFBO* a, TextureFBO* b){
    
}

void AppManager::runSimulation(double dt){
    advect(dt);
    diffuse(dt);
    addForce(dt);
    compPressure(dt);
    subPressureGrad(dt);
    
    CHECK_GL_ERRORS();
}

void AppManager::render(){
    double dt = 0.0005;//timer.elapsedAndRestart();
    runSimulation(dt);
    
    glViewport(0, 0, window_width*2, window_height*2);
    visualize->use();
    
    glUniform1i(visualize->getUniform("tex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, kernel0->getTexture());
    
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, NULL);
    glBindVertexArray(0);
    
    visualize->disuse();
    
    CHECK_GL_ERRORS();
}

void AppManager::createOpenGLContext(){
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    glfwWindowHint(GLFW_DEPTH_BITS, 32);
    glfwWindowHint(GLFW_RED_BITS, 8);
    glfwWindowHint(GLFW_GREEN_BITS, 8);
    glfwWindowHint(GLFW_BLUE_BITS, 8);
    glfwWindowHint(GLFW_ALPHA_BITS, 8);
    
    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(window_width, window_height, "GL App", NULL, NULL);
    if (window == NULL) {
        THROW_EXCEPTION("Failed to create window");
    }
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_callback);
    glfwSetCursorPosCallback(window, cursor_callback);

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    
    glewExperimental = GL_TRUE;
	GLenum glewErr = glewInit();
	if (glewErr != GLEW_OK) {
		std::stringstream err;
		err << "Error initializing GLEW: " << glewGetErrorString(glewErr);
		THROW_EXCEPTION(err.str());
	}
    
	// Unfortunately glewInit generates an OpenGL error, but does what it's
	// supposed to (setting function pointers for core functionality).
	// Lets do the ugly thing of swallowing the error....
    glGetError();
}

void AppManager::setOpenGLStates(){
    //glEnable(GL_DEPTH_TEST);
	//glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
    
    glViewport(0, 0, window_width, window_height);
	glClearColor(0.0, 0.0, 0.0, 1.0);
    
    CHECK_GL_ERRORS();
}

void AppManager::createProgram(){
    visualize   = new Program("kernel.vert","visualize.frag");
    copy        = new Program("kernel.vert","copy.frag");
    
    //Set uniforms

    CHECK_GL_ERRORS();
}

void AppManager::createVAO(){
    GLfloat quad_vertices[] =  {
		-1.0f, -1.0f,
        1.0f, -1.0f,
        1.0f,  1.0f,
		-1.0f,  1.0f,
	};
    vert = new BO<GL_ARRAY_BUFFER>(quad_vertices, sizeof(quad_vertices));
    
	GLubyte quad_indices[] = {
		0, 1, 2, //triangle 1
		2, 3, 0, //triangle 2
	};
    ind = new BO<GL_ELEMENT_ARRAY_BUFFER>(quad_indices, sizeof(quad_indices));
    
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    visualize->use();
    vert->bind();
    visualize->setAttributePointer("position", 2);
    ind->bind();
    glBindVertexArray(0);
    
    CHECK_GL_ERRORS();
}

void AppManager::createFBO(){
    kernel0 = new TextureFBO(Nx, Ny, GL_RGBA32F);
    kernel1 = new TextureFBO(Nx, Ny, GL_RGBA32F);
    
    CHECK_GL_ERRORS();
}

void AppManager::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

void AppManager::mouse_callback(GLFWwindow* window, int button, int action, int mods){
}

void AppManager::cursor_callback(GLFWwindow* window, double x, double y){
}