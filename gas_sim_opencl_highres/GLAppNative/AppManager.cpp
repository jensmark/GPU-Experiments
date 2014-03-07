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
    visualize(NULL),
    vert(NULL),
    ind(NULL),
    gamma(0.0f),
    time(0.0f),
    step(0){
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
    CLUtils::createContext(context);
    CLUtils::printDeviceInfo(context.device);
    
    setOpenGLStates();
    createBuffers();
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
    delete vert;
    delete ind;

    clReleaseKernel(compute_reconstruct);
    clReleaseKernel(evaluate_flux);
    clReleaseKernel(compute_RK);
    clReleaseKernel(compute_eigenvalues);
    clReleaseKernel(copy_domain);
    clReleaseKernel(prepare_render);
    clReleaseKernel(set_initial);
    clReleaseKernel(set_boundary_x);
    clReleaseKernel(set_boundary_y);
    
    delete compute_program;
    
    delete Sx_set;
    delete Sy_set;
    delete F_set;
    delete G_set;
    delete E_set;
    delete R_tex;
    for (size_t i = 0; i <= N_RK; i++) {
        delete Q_set[i];
    }
    
    
    CLUtils::releaseContext(context);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void AppManager::applyInitial(){
    cl_int err = CL_SUCCESS;
    gamma       = 1.4f;
    
    err |= clSetKernelArg(set_initial, 0, sizeof(cl_float), &gamma);
    err |= clSetKernelArg(set_initial, 1, sizeof(cl_float2),
                          glm::value_ptr(glm::vec2((1.0f/(float)Nx),(1.0f/(float)Ny))));
    err |= clSetKernelArg(set_initial, 2, sizeof(cl_mem), &(Q_set[N_RK]->getRef()));
    
    size_t global[] = {Nx,Ny};
    err |= clEnqueueNDRangeKernel(context.queue, set_initial, 2, NULL, global, NULL, 0, NULL, NULL);
    
    if(err != CL_SUCCESS) {
        THROW_EXCEPTION("Failed!");
    }
}

void AppManager::setBoundary(CLUtils::MO<CL_MEM_READ_WRITE>* Qn){
    cl_int err = CL_SUCCESS;
    
    err |= clSetKernelArg(set_boundary_x, 0, sizeof(cl_mem), &(Qn->getRef()));
    size_t globalx[] = {Nx+4};
    err |= clEnqueueNDRangeKernel(context.queue, set_boundary_x,
                                  1, NULL, globalx, NULL, 0, NULL, NULL);
    
    err |= clSetKernelArg(set_boundary_y, 0, sizeof(cl_mem), &(Qn->getRef()));
    size_t globaly[] = {Ny+4};
    err |= clEnqueueNDRangeKernel(context.queue, set_boundary_y,
                                  1, NULL, globaly, NULL, 0, NULL, NULL);
    
    if(err != CL_SUCCESS) {
        THROW_EXCEPTION("Failed!");
    }
}

float AppManager::computeDt(CLUtils::MO<CL_MEM_READ_WRITE>* Qn){
    cl_int err = CL_SUCCESS;
    static const float CFL = 0.5f;
    
    err |= clSetKernelArg(compute_eigenvalues, 0, sizeof(cl_mem), &(Qn->getRef()));
    err |= clSetKernelArg(compute_eigenvalues, 1, sizeof(cl_float), &gamma);
    err |= clSetKernelArg(compute_eigenvalues, 2, sizeof(cl_mem), &(E_set->getRef()));
    
    size_t global[] = {Nx,Ny};
    err |= clEnqueueNDRangeKernel(context.queue, compute_eigenvalues,
                                  2, NULL, global, NULL, 0, NULL, NULL);
    
    std::vector<cl_float> data((Nx+4)*(Ny+4));
    err |= clEnqueueReadBuffer(context.queue, E_set->getRef(), CL_TRUE, 0,
                               (Nx+4)*(Ny+4)*sizeof(cl_float), data.data(), 0, NULL, NULL);
    
    if(err != CL_SUCCESS) {
        THROW_EXCEPTION("Failed!");
    }
    
    float eig = -std::numeric_limits<float>().max();
    
    for (size_t x = 2; x < Nx; x++) {
        for (size_t y = 2; y < Ny; y++) {
            size_t k = (Nx * y + x);
            eig = glm::max(eig, data[k]);
        }
    }
    float dx = 1.0f/(float)Nx;
    float dy = 1.0f/(float)Ny;
    float dt = CFL*glm::min(dx/eig,dy/eig);
    
    std::cout << "Dt: " << dt << " Max eigenvalue: " << eig << std::endl << std::endl;
    
    return dt;
}

void AppManager::reconstruct(CLUtils::MO<CL_MEM_READ_WRITE>* Qn){
    cl_int err = CL_SUCCESS;

    err |= clSetKernelArg(compute_reconstruct, 0, sizeof(cl_mem), &(Qn->getRef()));
    err |= clSetKernelArg(compute_reconstruct, 1, sizeof(cl_mem), &(Sx_set->getRef()));
    err |= clSetKernelArg(compute_reconstruct, 2, sizeof(cl_mem), &(Sy_set->getRef()));
    
    size_t global[] = {Nx,Ny};
    err |= clEnqueueNDRangeKernel(context.queue, compute_reconstruct, 2,
                                  NULL, global, NULL, 0, NULL, NULL);
    
    if(err != CL_SUCCESS) {
        THROW_EXCEPTION("Failed!");
    }
}

void AppManager::evaluateFluxes(CLUtils::MO<CL_MEM_READ_WRITE>* Qn){
    cl_int err = CL_SUCCESS;
    
    err |= clSetKernelArg(evaluate_flux, 0, sizeof(cl_mem), &(Qn->getRef()));
    err |= clSetKernelArg(evaluate_flux, 1, sizeof(cl_mem), &(Sx_set->getRef()));
    err |= clSetKernelArg(evaluate_flux, 2, sizeof(cl_mem), &(Sy_set->getRef()));
    err |= clSetKernelArg(evaluate_flux, 3, sizeof(cl_float), &gamma);
    err |= clSetKernelArg(evaluate_flux, 4, sizeof(cl_mem), &(F_set->getRef()));
    err |= clSetKernelArg(evaluate_flux, 5, sizeof(cl_mem), &(G_set->getRef()));
    
    size_t global[] = {Nx,Ny};
    err |= clEnqueueNDRangeKernel(context.queue, evaluate_flux, 2,
                                  NULL, global, NULL, 0, NULL, NULL);
    
    if(err != CL_SUCCESS) {
        THROW_EXCEPTION("Failed!");
    }
}

void AppManager::computeRK(size_t n, float dt){
    cl_int err = CL_SUCCESS;
    
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
    
    err |= clSetKernelArg(compute_RK, 0, sizeof(cl_mem), &(Q_set[0]->getRef()));
    err |= clSetKernelArg(compute_RK, 1, sizeof(cl_mem), &(Q_set[n-1]->getRef()));
    err |= clSetKernelArg(compute_RK, 2, sizeof(cl_mem), &(F_set->getRef()));
    err |= clSetKernelArg(compute_RK, 3, sizeof(cl_mem), &(G_set->getRef()));
    err |= clSetKernelArg(compute_RK, 4, sizeof(cl_float2), glm::value_ptr(c[N_RK-1][n-1]));
    err |= clSetKernelArg(compute_RK, 5, sizeof(cl_float2),
                          glm::value_ptr(glm::vec2((1.0f/(float)Nx),(1.0f/(float)Ny))));
    err |= clSetKernelArg(compute_RK, 6, sizeof(cl_float), &dt);
    err |= clSetKernelArg(compute_RK, 7, sizeof(cl_mem), &(Q_set[n]->getRef()));
    
    size_t global[] = {Nx,Ny};
    err |= clEnqueueNDRangeKernel(context.queue, compute_RK, 2,
                                  NULL, global, NULL, 0, NULL, NULL);
    
    if(err != CL_SUCCESS) {
        THROW_EXCEPTION("Failed!");
    }

    
}

void AppManager::copy(CLUtils::MO<CL_MEM_READ_WRITE>* src,
          CLUtils::MO<CL_MEM_READ_WRITE>* dest){
    cl_int err = CL_SUCCESS;
    
    err |= clSetKernelArg(copy_domain, 0, sizeof(cl_mem), &(src->getRef()));
    err |= clSetKernelArg(copy_domain, 1, sizeof(cl_mem), &(dest->getRef()));
    
    size_t global[] = {Nx+4,Ny+4};
    err |= clEnqueueNDRangeKernel(context.queue, copy_domain, 2, NULL, global, NULL, 0, NULL, NULL);
    
    if(err != CL_SUCCESS) {
        THROW_EXCEPTION("Failed!");
    }
}

void AppManager::runKernel(){
    copy(Q_set[N_RK], Q_set[0]);
    
    float dt = computeDt(Q_set[0]);
    
    for (size_t n = 1; n <= N_RK; n++) {
        // apply boundary condition
        setBoundary(Q_set[n-1]);
        
        // reconstruct point values
        reconstruct(Q_set[n-1]);
        
        // evaluate fluxes
        evaluateFluxes(Q_set[n-1]);
        
        // compute RK
        computeRK(n, dt);
    }
    
    //debugDownload(Q_set[N_RK-1],true);
    
    time += dt;
    step++;
    
    
    CHECK_GL_ERRORS();
}

void AppManager::render(){
    runKernel();
    
    cl_int err = CL_SUCCESS;
    
    err |= clSetKernelArg(prepare_render, 0, sizeof(cl_mem), &Q_set[N_RK]->getRef());
    //err |= clSetKernelArg(prepare_render, 0, sizeof(cl_mem), &G_set->getRef());
    err |= clSetKernelArg(prepare_render, 1, sizeof(cl_image), &(R_tex->getRef()));
    
    size_t global[] = {Nx,Ny};
    err |= clEnqueueNDRangeKernel(context.queue, prepare_render, 2, NULL, global, NULL, 0, NULL, NULL);
    if(err != CL_SUCCESS) {
        THROW_EXCEPTION("Failed!");
    }
    
    clFinish(context.queue);
    
    glViewport(0, 0, window_width*2, window_height*2);
    visualize->use();
    
    glUniform1i(visualize->getUniform("tex"),0);
    glUniform2fv(visualize->getUniform("dXY"),1,
                 glm::value_ptr(glm::vec2((1.0f/(float)Nx),(1.0f/(float)Ny))));
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, NULL);
    glBindVertexArray(0);
    
    visualize->disuse();
    
    glFinish();
    CHECK_GL_ERRORS();
}

void AppManager::debugDownload(CLUtils::MO<CL_MEM_READ_WRITE>* Qn, bool dump){
    std::vector<cl_float> data((Nx+4)*(Ny+4)*4);
    clEnqueueReadBuffer(context.queue, Qn->getRef(), CL_TRUE, 0,
            (Nx+4)*(Nx+4)*sizeof(cl_float4), data.data(), 0, NULL, NULL);
    
    float maxrho    = -std::numeric_limits<float>().max();
    float minrho    = std::numeric_limits<float>().max();
    
    float maxu      = maxrho;
    float maxv      = maxrho;
    
    float minu      = minrho;
    float minv      = minrho;
    
    float maxE      = maxrho;
    float minE      = minrho;
    
    float rhoSum    = 0.0f;
    float rhouSum   = 0.0f;
    float rhovSum   = 0.0f;
    float ESum      = 0.0f;
    
    for (size_t x = 2; x < Nx; x++) {
        for (size_t y = 2; y < Ny; y++) {
            size_t k = (Nx * y + x)*4;
            
            rhoSum    += data[k];
            rhouSum   += data[k+1];
            rhovSum   += data[k+2];
            ESum      += data[k+3];
            
            maxrho = glm::max(maxrho,data[k]);
            minrho = glm::min(minrho,data[k]);
            
            maxu = glm::max(maxu,data[k+1]/data[k]);
            minu = glm::min(minu,data[k+1]/data[k]);
            
            maxv = glm::max(maxv,data[k+2]/data[k]);
            minv = glm::min(minv,data[k+2]/data[k]);
            
            maxE = glm::max(maxE,data[k+3]);
            minE = glm::min(minE,data[k+3]);
        }
    }
    
    std::cout << "Debug information @ s " << step << " t " << time << ": " << std::endl <<
    "Value Range: " << std::endl <<
    "p range: [" << minrho << "," << maxrho << "]" << std::endl <<
    "u range: [" << minu << "," << maxu << "]" << std::endl <<
    "v range: [" << minv << "," << maxv << "]" << std::endl <<
    "E range: [" << minE << "," << maxE << "]" << std::endl << std::endl <<
    "Value summation" << std::endl <<
    "p summation: " << rhoSum << std::endl <<
    "pu summation: " << rhouSum << std::endl <<
    "pv summation: " << rhovSum << std::endl <<
    "E summation: " << ESum << std::endl << std::endl;
    
    if(!dump){
        return;
    }
    
    std::cout << "Data: [";
    for (size_t y = 0; y < Ny+4; y++) {
        std::cout << "[" << std::endl;
        for (size_t x = 0; x < Nx+4; x++) {
            size_t k = ((Nx+4) * y + x)*4;
            std::cout << "[" << x << "," << y << "](" <<
            data[k] << "," <<
            data[k+1] << "," <<
            data[k+2] << "," <<
            data[k+3] << "), " << std::endl;
        }
        std::cout << "]";
    }
    std::cout << "]" << std::endl << std::endl;
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
    
    std::stringstream ss;
    ss << "-D Nx=" << Nx << " -D Ny=" << Ny;
    std::string options = ss.str();
    compute_program     = new CLUtils::Program(context, "compute_program.cl", &options);
    compute_reconstruct = compute_program->createKernel("piecewiseReconstruction");
    evaluate_flux       = compute_program->createKernel("computeNumericalFlux");
    compute_RK          = compute_program->createKernel("computeRK");
    compute_eigenvalues = compute_program->createKernel("eigenvalue");
    copy_domain         = compute_program->createKernel("copy");
    prepare_render      = compute_program->createKernel("copyToTexture");
    set_initial         = compute_program->createKernel("setInitial");
    set_boundary_x      = compute_program->createKernel("setBoundsX");
    set_boundary_y      = compute_program->createKernel("setBoundsY");

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

void AppManager::createBuffers(){
    
    // Initialize all buffers with 2 ghost cell on each edge
    for (size_t i = 0; i <= N_RK; i++) {
        Q_set[i] = new CLUtils::MO<CL_MEM_READ_WRITE>(context, (Nx+4)*(Ny+4)*sizeof(cl_float4), NULL);
    }
    Sx_set = new CLUtils::MO<CL_MEM_READ_WRITE>(context, (Nx+4)*(Ny+4)*sizeof(cl_float4), NULL);
    Sy_set = new CLUtils::MO<CL_MEM_READ_WRITE>(context, (Nx+4)*(Ny+4)*sizeof(cl_float4), NULL);
    F_set  = new CLUtils::MO<CL_MEM_READ_WRITE>(context, (Nx+4)*(Ny+4)*sizeof(cl_float4), NULL);
    G_set  = new CLUtils::MO<CL_MEM_READ_WRITE>(context, (Nx+4)*(Ny+4)*sizeof(cl_float4), NULL);
    E_set  = new CLUtils::MO<CL_MEM_WRITE_ONLY>(context, (Nx+4)*(Ny+4)*sizeof(cl_float), NULL);
    
    // We dont need to visualize ghost cells
    R_tex  = new CLUtils::ImageBuffer<CL_MEM_READ_WRITE>(context, tex, (Nx), (Ny), NULL);
    
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