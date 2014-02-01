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
    lax_f(NULL),
    visualize(NULL),
    copy(NULL),
    vert(NULL),
    ind(NULL),
    kernel0(NULL),
    kernel1(NULL),
    gamma(0.0f){
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
    
    applyInitial();
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
    delete lax_f;
    delete visualize;
    delete copy;
    delete vert;
    delete ind;
    delete kernel0;
    delete kernel1;
    
    glfwDestroyWindow(window);
    glfwTerminate();
}

float E(float rho, float u, float v, float gamma, float p){
    return 0.5*rho*(u*u+v*v)+p/(gamma-1.0f);
}

void AppManager::applyInitial(){
    gamma       = 1.4f;
    
    GLuint tex;
    glGenTextures(1, &tex);
    
    glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Initialize grid with initial data
    std::vector<GLfloat> data(Nx*Ny*4);
    
    for (size_t i = 0; i < Nx; i++) {
        for (size_t j = 0; j < Ny; j++) {
            // temp
            size_t k = (Nx * j + i)*4;
            data[k] = 0.1f;
            
            // populate circle
            const glm::vec2 center = glm::vec2(0.3f,0.5f);
            const float radius = 0.2f;
            
            if (glm::distance(glm::vec2((float)i/(float)Nx,(float)j/(float)Ny), center) <= radius) {
                data[k]     = 1.0f;
                //data[k+1]   = 1.0f*-10.0f;
                //data[k+2]   = 0.8f*10.0f;
                data[k+3]   = E(data[k], data[k+1], data[k+2], gamma, 1.0f);
            }
            
            /*glm::vec2 p((float)i/(float)Nx,(float)j/(float)Ny);
            if (p.x < center.x + radius &&
                p.x > center.x - radius &&
                p.y < center.y + radius &&
                p.y > center.y - radius){
                data[k] = 0.8f;
                //data[k+1]   = 0.8*-10.0f;
                data[k+3]   = E(data[k], data[k+1], data[k+2], gamma, 100.0f);
            }*/
            
            else{
                data[k+3]   = E(data[k], data[k+1], data[k+2], gamma, 0.1f);
            }
        }
    }
    
    // initial shockwave
    for (size_t i = 0; i < Ny; i++) {
        float x     = 0.0f;
        size_t k    = (Nx * i + (size_t)(x*(float)Nx))*4;
        data[k]     = 1.0f;
        data[k+1]   = data[k]*10.0f;
        data[k+3]   = E(data[k], data[k+1], data[k+2], gamma, 1000.0f);
    }
     
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, Nx, Ny,
                 0, GL_RGBA, GL_FLOAT, data.data());
    
    CHECK_GL_ERRORS();
    
    // Render initial grid to kernel0
    kernel0->bind();
    glViewport(0, 0, Nx, Ny);
    
    copy->use();
    
    //set uniforms
    glUniform1i(copy->getUniform("QTex"), 0);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, NULL);
    glBindVertexArray(0);
    
    copy->disuse();
    kernel0->unbind();
    
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &tex);
    
    CHECK_GL_ERRORS();
}

void AppManager::runKernel(double dt){
    kernel1->bind();
    glViewport(0, 0, Nx, Ny);
    
    lax_f->use();
    
    float rx = (float)dt/(1.0f/(float)Nx);
    float ry = (float)dt/(1.0f/(float)Ny);
    
    //set uniforms
    glUniform1f(lax_f->getUniform("rx"), rx);
    glUniform1f(lax_f->getUniform("ry"), ry);
    glUniform1f(lax_f->getUniform("gamma"), gamma);
    glUniform1i(lax_f->getUniform("QTex"), 0);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, kernel0->getTexture());
    
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, NULL);
    glBindVertexArray(0);
    
    lax_f->disuse();
    kernel1->unbind();
  
    // Flip kernels for next iteration
    TextureFBO* temp = kernel0;
    kernel0 = kernel1;
    kernel1 = temp;
   
    CHECK_GL_ERRORS();
    
    /* DOWNLOAD RESULTS */
    
    glBindTexture(GL_TEXTURE_2D, kernel0->getTexture());
    
    std::vector<GLfloat> data(Nx*Ny*4);
    glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_FLOAT,&data[0]);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    float max = 0.0;
    for (size_t x = 0; x < Nx; x++) {
        for (size_t y = 0; y < Ny; y++) {
            max = glm::max(max,data[(Nx * y + x)*4]);
        }
    }
    
    std::cout << max << std::endl;
    
    /* DOWNLOAD END */
    
    CHECK_GL_ERRORS();
}

void AppManager::render(){
    double dt = 0.00002;//timer.elapsedAndRestart();
    runKernel(dt);
    
    glViewport(0, 0, window_width*2, window_height*2);
    visualize->use();
    
    float rx = (1.0f/(float)Nx);
    float ry = (1.0f/(float)Ny);
    
    glUniform1f(visualize->getUniform("rx"), rx);
    glUniform1f(visualize->getUniform("ry"), ry);
    
    glUniform1i(visualize->getUniform("QTex"), 0);
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
    lax_f       = new Program("kernel.vert","lax-f.frag");
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