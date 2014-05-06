//
//  SimulatorGLEuler
//  GLAppNative
//
//  Created by Jens Kristoffer Reitan Markussen on 28.12.13.
//  Copyright (c) 2013 Jens Kristoffer Reitan Markussen. All rights reserved.
//

#include "SimulatorGLEuler.h"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>

SimulatorGLEuler::SimulatorGLEuler(){
    
}

SimulatorGLEuler::~SimulatorGLEuler(){
    delete initialK;
    delete copy;
    delete vert;
    delete ind;
    delete runge_kutta;
    delete bilinear_recon;
    delete flux_evaluator;
    delete eigen;
    
    for (size_t i = 0; i <= N_RK; i++) {
        delete kernelRK[i];
    }
    delete reconstructKernel;
    delete fluxKernel;
}

void SimulatorGLEuler::init(size_t Nx, size_t Ny, std::string initialKernel){
    this->Nx = Nx;
    this->Ny = Ny;
    this->gamma = 1.4;
    this->time = 0.0f;
    
    std::cout << "Simulating Euler using OpenGL Shaders on GPU" << std::endl;
    
    createFBO();
    //createProgram(initialKernel);
    createProgram("initial_shock");
    createVAO();
    
    applyInitial();
}
SimDetail SimulatorGLEuler::simulate(){
    copyTexture(kernelRK[N_RK]->getTexture(), kernelRK[0]);
    
    float dt = computeDt(kernelRK[0]);
    
    timer.restart();
    SimDetail detail;
    
    for (size_t n = 1; n <= N_RK; n++) {
        // apply boundary condition
        //setBoundary(kernelRK[n-1]);
        
        // reconstruct point values
        reconstruct(kernelRK[n-1]);
        
        // evaluate fluxes
        evaluateFluxes(kernelRK[n-1]);
        
        // compute RK
        computeRK(n, dt);
    }
    
    glFinish();
    
    detail.sim_time = timer.elapsed();
    detail.dt = dt;
    time+=dt;
    detail.time = time;
    
    CHECK_GL_ERRORS();
    
    return detail;
}

size_t SimulatorGLEuler::getTexture(){
    return kernelRK[N_RK]->getTexture();
}
std::vector<float> SimulatorGLEuler::getData(){
    THROW_EXCEPTION("Not Implemented");
}

void SimulatorGLEuler::createProgram(std::string initial){
    copy            = new GLUtils::Program("res/shaders/kernel.vert","res/shaders/copy.frag");
    flux_evaluator  = new GLUtils::Program("res/shaders/kernel.vert","res/shaders/comp_flux.frag");
    runge_kutta     = new GLUtils::Program("res/shaders/kernel.vert","res/shaders/RK.frag");
    bilinear_recon  = new GLUtils::Program("res/shaders/kernel.vert","res/shaders/bilin_reconstruction.frag");
    eigen           = new GLUtils::Program("res/shaders/kernel.vert","res/shaders/eigenvalue.frag");
    
    std::stringstream ss;
    ss << "res/shaders/" << initial << ".frag";
    initialK        = new GLUtils::Program("res/shaders/kernel.vert",ss.str());
    
    //Set uniforms
    
    CHECK_GL_ERRORS();
}

void SimulatorGLEuler::createVAO(){
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
    vert = new GLUtils::BO<GL_ARRAY_BUFFER>(quad_vertices, sizeof(quad_vertices));
    
	GLubyte quad_indices[] = {
		0, 1, 2, //triangle 1
		2, 3, 0, //triangle 2
	};
    ind = new GLUtils::BO<GL_ELEMENT_ARRAY_BUFFER>(quad_indices, sizeof(quad_indices));
    
    glBindVertexArray(vao[0]);
    copy->use();
    vert->bind();
    copy->setAttributePointer("position", 2, GL_FLOAT, GL_FALSE, 16, BUFFER_OFFSET(0));
    copy->setAttributePointer("tex", 2, GL_FLOAT, GL_FALSE, 16, BUFFER_OFFSET(8));
    ind->bind();
    glBindVertexArray(0);
}

void SimulatorGLEuler::createFBO(){
    for (size_t i = 0; i <= N_RK; i++) {
        kernelRK[i] = new TextureFBO(Nx,Ny);
    }
    reconstructKernel   = new TextureFBO(Nx,Ny,2);
    fluxKernel          = new TextureFBO(Nx,Ny,2);
    dtKernel            = new TextureFBO(Nx,Ny);
    
    CHECK_GL_ERRORS();
}

void SimulatorGLEuler::applyInitial(){
    kernelRK[N_RK]->bind();
    glViewport(0, 0, Nx, Ny);
    initialK->use();
    
    glUniform1f(initialK->getUniform("gamma"),gamma);
    glUniform2fv(initialK->getUniform("dxy"),1,glm::value_ptr(glm::vec2(1.0f/(float)Nx,1.0f/(float)Ny)));
    glUniform2fv(initialK->getUniform("Nxy"),1,glm::value_ptr(glm::vec2((float)Nx,(float)Ny)));
    
    glBindVertexArray(vao[0]);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, NULL);
    glBindVertexArray(0);
    
    initialK->disuse();
    kernelRK[N_RK]->unbind();
}

void SimulatorGLEuler::setBoundary(TextureFBO* Qn){
    THROW_EXCEPTION("Not Implemented");
}

float SimulatorGLEuler::computeDt(TextureFBO* Qn){
    static const float CFL = 0.5f;
    
    dtKernel->bind();
    glViewport(0, 0, Nx, Ny);
    eigen->use();
    
    glUniform1i(eigen->getUniform("QTex"),0);
    glUniform1f(eigen->getUniform("gamma"),gamma);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Qn->getTexture());
    
    glBindVertexArray(vao[0]);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, NULL);
    glBindVertexArray(0);
    
    eigen->disuse();
    dtKernel->unbind();
    
    glBindTexture(GL_TEXTURE_2D, dtKernel->getTexture());
    
    std::vector<GLfloat> data(Nx*Ny*4);
    glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_FLOAT,&data[0]);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    float eig = -std::numeric_limits<float>().max();
    
    for (size_t x = 0; x < Nx; x++) {
        for (size_t y = 0; y < Ny; y++) {
            size_t k = (Nx * y + x)*4;
            eig = glm::max(eig, data[k]);
        }
    }
    float dx = 1.0f/(float)Nx;
    float dy = 1.0f/(float)Ny;
    float dt = CFL*glm::min(dx/eig,dy/eig);
    
    return dt;
}

void SimulatorGLEuler::reconstruct(TextureFBO* Qn){
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

void SimulatorGLEuler::evaluateFluxes(TextureFBO* Qn){
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
    glBindTexture(GL_TEXTURE_2D, reconstructKernel->getTexture(1));
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, reconstructKernel->getTexture(0));
    glActiveTexture(GL_TEXTURE3);
    
    glBindVertexArray(vao[0]);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, NULL);
    glBindVertexArray(0);
    
    flux_evaluator->disuse();
    fluxKernel->unbind();
}

void SimulatorGLEuler::computeRK(size_t n, float dt){
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
    glUniform1i(runge_kutta->getUniform("QKTex"),1);
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
    glBindTexture(GL_TEXTURE_2D, fluxKernel->getTexture(1));
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, fluxKernel->getTexture(0));
    
    glBindVertexArray(vao[0]);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, NULL);
    glBindVertexArray(0);
    
    runge_kutta->disuse();
    kernelRK[n]->unbind();
}

void SimulatorGLEuler::copyTexture(GLint source, TextureFBO* dest){
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
