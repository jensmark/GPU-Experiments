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
#include <glm/gtc/type_ptr.hpp>

AppManager::AppManager():
    //lax_f(NULL),
    visualize(NULL),
    copy(NULL),
    vert(NULL),
    ind(NULL),
    gamma(0.0f),
    pressure(0.0f),
    time(0.0f){
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
    delete visualize;
    delete copy;
    delete vert;
    delete ind;
    delete b_vert;
    delete runge_kutta;
    delete bilinear_recon;
    delete flux_evaluator;
    delete boundary;
    
    for (size_t i = 0; i <= N_RK; i++) {
        delete kernelRK[i];
    }
    delete reconstructKernel;
    delete fluxKernel;
    
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Initialize grid with initial data
    std::vector<GLfloat> data(Nx*Ny*4);
    
    /*
    for (size_t i = 0; i < Nx; i++) {
        for (size_t j = 0; j < Ny; j++) {
            // temp
            size_t k = (Nx * j + i)*4;
            data[k] = 0.01f;
            
            // populate circle
            const glm::vec2 center = glm::vec2(0.3f,0.5f);
            const float radius = 0.2f;
            
            if (glm::distance(glm::vec2((float)i/(float)Nx,(float)j/(float)Ny), center) <= radius) {
                data[k]     = 1.0f;
                data[k+3]   = E(data[k], 0.0f, 0.0f, gamma, 1.0f);
            }
            
            else{
                data[k+3]   = E(data[k], 0.0f, 0.0f, gamma, 0.01f);
            }
        }
    }
    
    // initial shockwave
    for (size_t i = 0; i < Ny; i++) {
        float x     = 0.0f;
        size_t k    = (Nx * i + (size_t)(x*(float)Nx))*4;
        data[k]     = 1.0f;
        data[k+1]   = data[k]*1.0f;
        data[k+3]   = E(data[k], 1.0f, 0.0f, gamma, 1.0f);
    }
    */
    
    
    // 2D Riemann condition
    glm::vec4 Q[4];
    
    //
    // Riemann problem 1
    //
    Q[0].x = 1.0f;
    Q[0].y = 0.0f;
    Q[0].z = 0.0f;
    Q[0].w = E(1.0f, 0.0f, 0.0f, gamma, 1.0f);
    
    Q[1].x = 0.5197f;
    Q[1].y = 0.5197f*-0.7259f;
    Q[1].z = 0.0f;
    Q[1].w = E(0.5197f, -0.7269f, 0.0f, gamma, 0.4f);
    
    Q[2].x = 1.0f;
    Q[2].y = 1.0f*-0.7269f;
    Q[2].z = 1.0f*-0.7269f;
    Q[2].w = E(1.0f, -0.7269f, -0.7269f, gamma, 1.0f);
    
    Q[3].x = 0.5197f;
    Q[3].y = 0.0f;
    Q[3].z = 0.5197f*-0.7259f;
    Q[3].w = E(0.5197f, 0.0f, -0.7269f, gamma, 0.4f);

    
    
    for (size_t i = 0; i < Nx; i++) {
        for (size_t j = 0; j < Ny; j++) {
            size_t k = (Nx * j + i)*4;
            glm::vec2 coord((float)i/(float)Nx,(float)j/(float)Ny);
            
            if (coord.x >= 0.5f && coord.y >= 0.5f) {
                data[k]     = Q[0].x;
                data[k+1]   = Q[0].y;
                data[k+2]   = Q[0].z;
                data[k+3]   = Q[0].w;
            }
            else if (coord.x <= 0.5f && coord.y >= 0.5f){
                data[k]     = Q[1].x;
                data[k+1]   = Q[1].y;
                data[k+2]   = Q[1].z;
                data[k+3]   = Q[1].w;
            }
            else if (coord.x <= 0.5f && coord.y <= 0.5f){
                data[k]     = Q[2].x;
                data[k+1]   = Q[2].y;
                data[k+2]   = Q[2].z;
                data[k+3]   = Q[2].w;
            }
            else if (coord.x >= 0.5f && coord.y <= 0.5f){
                data[k]     = Q[3].x;
                data[k+1]   = Q[3].y;
                data[k+2]   = Q[3].z;
                data[k+3]   = Q[3].w;
            }
        }
    }
    
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, Nx, Ny,
                 0, GL_RGBA, GL_FLOAT, data.data());
    
    CHECK_GL_ERRORS();
    
    copyTexture(tex, kernelRK[N_RK]);
    glDeleteTextures(1, &tex);
}

void AppManager::setBoundary(TextureFBO* Qn){
    Qn->bind();
    glViewport(0, 0, Nx, Ny);
    boundary->use();
    
    //glUniform1i(boundary->getUniform("QTex"),0);
    
    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, Qn->getTexture());
    
    glBindVertexArray(vao[1]);
	glDrawArrays(GL_LINES, 0, 8);
	glBindVertexArray(0);
    
    boundary->disuse();
    Qn->unbind();
}

void AppManager::reconstruct(TextureFBO* Qn){
    reconstructKernel->bind();
    glViewport(0, 0, Nx, Ny);
    bilinear_recon->use();
    
    glUniform1i(bilinear_recon->getUniform("QTex"),0);
    
    //float dx = (1.0f/(float)Nx);
    //float dy = (1.0f/(float)Ny);
    
    //glUniform1f(runge_kutta->getUniform("dx"),dx);
    //glUniform1f(runge_kutta->getUniform("dy"),dy);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Qn->getTexture());
    
    glBindVertexArray(vao[0]);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, NULL);
    glBindVertexArray(0);
    
    bilinear_recon->disuse();
    reconstructKernel->unbind();
}

void AppManager::evaluateFluxes(TextureFBO* Qn){
    fluxKernel->bind();
    glViewport(0, 0, Nx, Ny);
    flux_evaluator->use();
    
    glUniform1i(flux_evaluator->getUniform("QTex"),0);
    glUniform1i(flux_evaluator->getUniform("SxTex"),1);
    glUniform1i(flux_evaluator->getUniform("SyTex"),2);
    
    glUniform1f(flux_evaluator->getUniform("gamma"),gamma);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Qn->getTexture());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, reconstructKernel->getTexture(0));
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, reconstructKernel->getTexture(1));
    glActiveTexture(GL_TEXTURE3);
    
    glBindVertexArray(vao[0]);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, NULL);
    glBindVertexArray(0);
    
    flux_evaluator->disuse();
    fluxKernel->unbind();
}

void AppManager::computeRK(size_t n, float dt){
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
    
    kernelRK[n]->bind();
    glViewport(0, 0, Nx, Ny);
    runge_kutta->use();
    
    glUniform1i(runge_kutta->getUniform("QNTex"),0);
    glUniform1i(runge_kutta->getUniform("QTex"),1);
    glUniform1i(runge_kutta->getUniform("FHalfTex"),2);
    glUniform1i(runge_kutta->getUniform("GHalfTex"),3);
    
    glUniform2fv(runge_kutta->getUniform("c"),1,glm::value_ptr(c[N_RK-1][n-1]));
    
    float dx = (1.0f/(float)Nx);
    float dy = (1.0f/(float)Ny);
    
    glUniform1f(runge_kutta->getUniform("dt"),dt);
    glUniform1f(runge_kutta->getUniform("dx"),dx);
    glUniform1f(runge_kutta->getUniform("dy"),dy);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, kernelRK[0]->getTexture());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, kernelRK[n-1]->getTexture());
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, fluxKernel->getTexture(0));
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, fluxKernel->getTexture(1));
    
    glBindVertexArray(vao[0]);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, NULL);
    glBindVertexArray(0);
    
    runge_kutta->disuse();
    kernelRK[n]->unbind();
}

void AppManager::copyTexture(GLint source, TextureFBO* dest){
    
    dest->bind();
    glViewport(0, 0, Nx, Ny);
    
    copy->use();
    
    //set uniforms
    glUniform1i(copy->getUniform("QTex"), 0);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, source);
    
    glBindVertexArray(vao[0]);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, NULL);
    glBindVertexArray(0);
    
    copy->disuse();
    dest->unbind();
    glBindTexture(GL_TEXTURE_2D, 0);
    
    CHECK_GL_ERRORS();
}

void AppManager::runKernel(double dt){
    debugDownload();
    copyTexture(kernelRK[N_RK]->getTexture(), kernelRK[0]);
    
    for (size_t n = 1; n <= N_RK; n++) {
        // apply boundary condition
        //setBoundary(kernelRK[n-1]);
    
        // reconstruct point values
        reconstruct(kernelRK[n-1]);
        
        // evaluate fluxes
        evaluateFluxes(kernelRK[n-1]);
        
        // compute RK
        computeRK(n, (float)dt);
    }
    time += (float)dt;
    
    debugDownload();
    
    CHECK_GL_ERRORS();
}

void AppManager::render(){
    double dt = 1e-10f;//timer.elapsedAndRestart();
    runKernel(dt);
    
    glViewport(0, 0, window_width*2, window_height*2);
    visualize->use();
    
    float dx = (1.0f/(float)Nx);
    float dy = (1.0f/(float)Ny);
    
    glUniform1f(visualize->getUniform("dx"),dx);
    glUniform1f(visualize->getUniform("dy"),dy);
    
    glUniform1i(visualize->getUniform("QTex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, kernelRK[N_RK]->getTexture(0));
    
    glBindVertexArray(vao[0]);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, NULL);
    glBindVertexArray(0);
    
    visualize->disuse();
    
    CHECK_GL_ERRORS();
}

void AppManager::debugDownload(){
    glBindTexture(GL_TEXTURE_2D, kernelRK[N_RK]->getTexture());
    //glBindTexture(GL_TEXTURE_2D, fluxKernel->getTexture(1));
    
    std::vector<GLfloat> data(Nx*Ny*4);
    glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_FLOAT,&data[0]);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    float maxrho    = -std::numeric_limits<float>().max();
    float minrho    = std::numeric_limits<float>().max();
    
    float maxu      = maxrho;
    float maxv      = maxrho;
    
    float minu      = minrho;
    float minv      = minrho;
    
    float maxE      = maxrho;
    float minE      = minrho;
    
    for (size_t x = 0; x < Nx; x++) {
        for (size_t y = 0; y < Ny; y++) {
            size_t k = (Nx * y + x)*4;
            
            maxrho = glm::max(maxrho,data[k]);
            minrho = glm::min(minrho,data[k]);
            
            maxu = glm::max(maxu,data[k+1]);
            minu = glm::min(minu,data[k+1]);
            
            maxv = glm::max(maxv,data[k+2]);
            minv = glm::min(minv,data[k+2]);
            
            maxE = glm::max(maxE,data[k+3]);
            minE = glm::min(minE,data[k+3]);
        }
    }
    
    std::cout << "Debug information @ " << time << ": " << std::endl <<
        "Density range: [" << minrho << "," << maxrho << "]" << std::endl <<
        "Velocity u range: [" << minu << "," << maxu << "]" << std::endl <<
        "Velocity v range: [" << minv << "," << minu << "]" << std::endl <<
        "Energy range: [" << minE << "," << maxE << "]" << std::endl << std::endl;
    
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
    glLineWidth(1.0f);
    
    glViewport(0, 0, window_width, window_height);
	glClearColor(0.0, 0.0, 0.0, 0.0);
    
    CHECK_GL_ERRORS();
}

void AppManager::createProgram(){
    visualize       = new Program("kernel.vert","visualize.frag");
    copy            = new Program("kernel.vert","copy.frag");
    flux_evaluator  = new Program("kernel.vert","comp_flux.frag");
    runge_kutta     = new Program("kernel.vert","RK.frag");
    bilinear_recon  = new Program("kernel.vert","bilin_reconstruction.frag");
    boundary        = new Program("boundary_kernel.vert","boundary.frag");
    
    //Set uniforms

    CHECK_GL_ERRORS();
}

void AppManager::createVAO(){
    glGenVertexArrays(2, &vao[0]);
    
    float dx = 0.0f;//1.0f/(float)Nx;
    float dy = 0.0f;//1.0f/(float)Ny;
    
    GLfloat quad_vertices[] =  {
		// bottom-left (0)
        -1.0f,          -1.0f,
        0.0f+dx,   0.0f+dy,
        
        // bottom-right (1)
        1.0f,           -1.0f,
        1.0f-dx,   0.0f+dy,
        
        // top-left (2)
        1.0f,           1.0f,
        1.0f-dx,   1.0f-dy,
        
        // top-right (3)
        -1.0f,          1.0f,
        0.0f+dx,   1.0f-dy,
	};
    vert = new BO<GL_ARRAY_BUFFER>(quad_vertices, sizeof(quad_vertices));
    
	GLubyte quad_indices[] = {
		0, 1, 2, //triangle 1
		2, 3, 0, //triangle 2
	};
    ind = new BO<GL_ELEMENT_ARRAY_BUFFER>(quad_indices, sizeof(quad_indices));
    
    glBindVertexArray(vao[0]);
    visualize->use();
    vert->bind();
    visualize->setAttributePointer("position", 2, GL_FLOAT, GL_FALSE, 16, BUFFER_OFFSET(0));
    visualize->setAttributePointer("tex", 2, GL_FLOAT, GL_FALSE, 16, BUFFER_OFFSET(8));
    ind->bind();
    glBindVertexArray(0);
    
    GLfloat boundsData[] = {
       // bottom
        -1.0f, -1.0f,
        1.0f, -1.0f,
        
        // top
        -1.0f,  1.0f,
        1.0f,  1.0f,
        
        // left
        -1.0f,  1.0f,
        -1.0f, -1.0f,
        
        // right
        1.0f,  1.0f,
        1.0f, -1.0f,
    };
    b_vert = new BO<GL_ARRAY_BUFFER>(boundsData, sizeof(boundsData));
    
    glBindVertexArray(vao[1]);
    b_vert->bind();
	boundary->setAttributePointer("position", 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	b_vert->unbind();
    glBindVertexArray(0);
    
    CHECK_GL_ERRORS();
}

void AppManager::createFBO(){
    for (size_t i = 0; i <= N_RK; i++) {
        kernelRK[i] = new TextureFBO(Nx,Ny);
    }
    reconstructKernel   = new TextureFBO(Nx,Ny,2);
    fluxKernel          = new TextureFBO(Nx,Ny,2);
    
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