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
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>

// Static decleration
VirtualTrackball AppManager::trackball;

AppManager::AppManager():
    visualize(NULL),
    surface_vert(NULL),
    surface_ind(NULL),
    gravity(9.81f),
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
    
    camera.projection = glm::perspective(45.0f,
            window_width / (float) window_height, 1.0f, 50.0f);
	camera.view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.0f));
    
    model = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f,-0.5f,-0.5f));
    
    trackball.setWindowSize(window_width, window_height);
    
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
    delete surface_vert;
    delete surface_ind;

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
    
    err |= clSetKernelArg(set_initial, 0, sizeof(cl_float), &gravity);
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
    size_t globalx[] = {Nx};
    err |= clEnqueueNDRangeKernel(context.queue, set_boundary_x,
                                  1, NULL, globalx, NULL, 0, NULL, NULL);
    
    err |= clSetKernelArg(set_boundary_y, 0, sizeof(cl_mem), &(Qn->getRef()));
    size_t globaly[] = {Ny};
    err |= clEnqueueNDRangeKernel(context.queue, set_boundary_y,
                                  1, NULL, globaly, NULL, 0, NULL, NULL);
    
    if(err != CL_SUCCESS) {
        THROW_EXCEPTION("Failed!");
    }
}

float AppManager::computeDt(CLUtils::MO<CL_MEM_READ_WRITE>* Qn){
    cl_int err = CL_SUCCESS;
    static const float CFL = 0.8f;
    
    err |= clSetKernelArg(compute_eigenvalues, 0, sizeof(cl_mem), &(Qn->getRef()));
    err |= clSetKernelArg(compute_eigenvalues, 1, sizeof(cl_float), &gravity);
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
    
    for (size_t x = 2; x < Nx+2; x++) {
        for (size_t y = 2; y < Ny+2; y++) {
            size_t k = ((Nx+4) * y + x);
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
    
    size_t global[] = {Nx+2,Ny+2};
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
    err |= clSetKernelArg(evaluate_flux, 3, sizeof(cl_float), &gravity);
    err |= clSetKernelArg(evaluate_flux, 4, sizeof(cl_mem), &(F_set->getRef()));
    err |= clSetKernelArg(evaluate_flux, 5, sizeof(cl_mem), &(G_set->getRef()));
    
    size_t global[] = {Nx+1,Ny+1};
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
    setBoundary(Q_set[N_RK]);
    copy(Q_set[N_RK], Q_set[0]);
    
    dumpJSON("test");
    
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
    
    time += dt;
    step++;
    
    debugDownload(Q_set[N_RK], false);
    //debugDownload(G_set, true);
    
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
    
    std::vector<cl_float4> data((Nx+4)*(Ny+4));
    clEnqueueReadBuffer(context.queue, Q_set[N_RK]->getRef(), CL_TRUE, 0,
                        (Nx+4)*(Nx+4)*sizeof(cl_float4), data.data(), 0, NULL, NULL);
    h_max    = -std::numeric_limits<float>().max();
    h_min    = std::numeric_limits<float>().max();
    for (size_t x = 2; x < Nx+2; x++) {
        for (size_t y = 2; y < Ny+2; y++) {
            size_t k = ((Nx+4) * y + x);
            
            h_max = glm::max(h_max,(glm::abs(data[k].y/data[k].x)+glm::abs(data[k].z/data[k].x)));
            h_min = glm::min(h_min,(glm::abs(data[k].y/data[k].x)+glm::abs(data[k].z/data[k].x)));
        }
    }
    std::cout << "h max: " << h_max << " h min: " << h_min << std::endl;
    
    clFinish(context.queue);
    int width, height;
    glfwGetFramebufferSize(window,&width,&height);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    visualize->use();
    
    glm::mat4 view_matrix_new = camera.view*trackball.getTransform();
    
    glUniform1i(visualize->getUniform("tex"),0);
    glUniform2fv(visualize->getUniform("dxy"),1,glm::value_ptr(glm::vec2((1.0f/(float)Nx),(1.0f/(float)Ny))));
    glUniform2fv(visualize->getUniform("h_minmax"),1,glm::value_ptr(glm::vec2((float)h_min,(float)h_max)));

    glUniformMatrix4fv(visualize->getUniform("projection_matrix"), 1, 0, glm::value_ptr(camera.projection));
    glUniformMatrix4fv(visualize->getUniform("model_matrix"), 1, 0, glm::value_ptr(model));
    glUniformMatrix4fv(visualize->getUniform("view_matrix"), 1, 0, glm::value_ptr(view_matrix_new));
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    
    glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(restart_token);
    
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLE_STRIP, indices_count, GL_UNSIGNED_INT, NULL);
    glBindVertexArray(0);
    
    visualize->disuse();
    
    glDisable(GL_PRIMITIVE_RESTART);
    
    glFinish();
    CHECK_GL_ERRORS();
}

void AppManager::dumpJSON(std::string filename){
    std::vector<cl_float> q_data((Nx+4)*(Ny+4)*4);
    std::vector<cl_float> f_data((Nx+4)*(Ny+4)*4);
    std::vector<cl_float> g_data((Nx+4)*(Ny+4)*4);
    std::vector<cl_float> sx_data((Nx+4)*(Ny+4)*4);
    std::vector<cl_float> sy_data((Nx+4)*(Ny+4)*4);
    
    
    clEnqueueReadBuffer(context.queue, Q_set[N_RK]->getRef(), CL_TRUE, 0,
                        (Nx+4)*(Nx+4)*sizeof(cl_float4), q_data.data(), 0, NULL, NULL);
    clEnqueueReadBuffer(context.queue, F_set->getRef(), CL_TRUE, 0,
                        (Nx+4)*(Nx+4)*sizeof(cl_float4), f_data.data(), 0, NULL, NULL);
    clEnqueueReadBuffer(context.queue, G_set->getRef(), CL_TRUE, 0,
                        (Nx+4)*(Nx+4)*sizeof(cl_float4), g_data.data(), 0, NULL, NULL);
    clEnqueueReadBuffer(context.queue, Sx_set->getRef(), CL_TRUE, 0,
                        (Nx+4)*(Nx+4)*sizeof(cl_float4), sx_data.data(), 0, NULL, NULL);
    clEnqueueReadBuffer(context.queue, Sy_set->getRef(), CL_TRUE, 0,
                        (Nx+4)*(Nx+4)*sizeof(cl_float4), sy_data.data(), 0, NULL, NULL);
    
    std::ofstream output;
    std::stringstream file;
    file << filename << "_" << step << ".json";
    
    output.unsetf ( std::ios::floatfield );
    output.precision(4);
    output.setf( std::ios::fixed, std:: ios::floatfield );
    
    output.open(file.str());
    output << "{";
    output << "\"frame\":" << step << ","
           << "\"time\":" << time << "," << std::endl;
    
    output << "\"cells\":[" << std::endl;
    for (int y = Ny+3; y >= 0; y--) {
        output << "[";
        for (int x = 0; x < Nx+4; x++) {
            output << "{";
            size_t k = ((Nx+4) * y + x)*4;
            
            size_t sk = ((Nx+4) * (glm::max((y-1),0)) + x)*4;
            size_t wk = ((Nx+4) * y + (glm::max((x-1),0)))*4;
            
            float h = q_data[k];
            
            
            output << "\"type\":";
            if(y <= 1 || y >= Ny+2){
                output << "\"G\"";
            }else if(x <= 1 || x >= Nx+2){
                output << "\"G\"";
            }else{
                output << "\"I\"";
            }
            output << ",";
            output << "\"h\":" << h << ",";
            output << "\"derived\":{\"Sx\":" << std::setw(4) << sx_data[k] <<
                ",\"Sy\":" << std::setw(4) << sy_data[k] << "},";
            output << "\"flux\":{\"N\":" << std::setw(4) << g_data[k] << ",\"S\":"
                << std::setw(4) << g_data[sk]
                    << ",\"E\":" << std::setw(4) << f_data[k] << ",\"W\":"
                << std::setw(4) << f_data[wk] << "}";
            output << "}";
            if(x != Nx+3){
                output << ",\t\t";
            }
        }
        output << "]";
        if(y != 0){
            output << ",";
        }
        output << std::endl;
    }
    output << "]";
    
    output << "}";
    output.unsetf ( std::ios::floatfield );
    output.close();
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
    
    float rhoSum    = 0.0f;
    float rhouSum   = 0.0f;
    float rhovSum   = 0.0f;
    
    for (size_t x = 2; x < Nx+2; x++) {
        for (size_t y = 2; y < Ny+2; y++) {
            size_t k = ((Nx+4) * y + x)*4;
            
            rhoSum    += data[k];
            rhouSum   += data[k+1];
            rhovSum   += data[k+2];
            
            maxrho = glm::max(maxrho,data[k]);
            minrho = glm::min(minrho,data[k]);
            
            maxu = glm::max(maxu,data[k+1]);
            minu = glm::min(minu,data[k+1]);
            
            maxv = glm::max(maxv,data[k+2]);
            minv = glm::min(minv,data[k+2]);
        }
    }
    
    std::cout << "Debug information @ s " << step << " t " << time << ": " << std::endl <<
    "Value Range: " << std::endl <<
    "h range: [" << minrho << "," << maxrho << "]" << std::endl <<
    "hu range: [" << minu << "," << maxu << "]" << std::endl <<
    "hv range: [" << minv << "," << maxv << "]" << std::endl << std::endl <<
    "Value summation" << std::endl <<
    "h summation: " << rhoSum << std::endl <<
    "hu summation: " << rhouSum << std::endl <<
    "hv summation: " << rhovSum << std::endl << std::endl;
    
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
            data[k+2] << "), " << std::endl;
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
    glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
    
    glViewport(0, 0, window_width, window_height);
	glClearColor(1.0, 1.0, 1.0, 1.0);
    
    CHECK_GL_ERRORS();
}

void AppManager::createProgram(){
    visualize   = new Program("heightmap.vert","surface.frag");
    
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
    float r = (float)Nx/(float)Ny;
    float d = glm::sqrt((float)(256*256+256*256));
    int w = (int)glm::max(d/(glm::sqrt(r*r+1.0))+1.0,10.0);
    int h = (int)glm::max(d/(glm::sqrt((1.0/(r*r))+1.0))+1.0,10.0);
    
    createTriangleStrip(h,w,surface_vert,surface_ind);
    
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    visualize->use();
    surface_vert->bind();
    visualize->setAttributePointer("position", 2);
    surface_ind->bind();
    glBindVertexArray(0);
    
    CHECK_GL_ERRORS();
}

void AppManager::createBuffers(){
    
    // Initialize all buffers with 2 ghost cell on each edge
    for (size_t i = 0; i <= N_RK; i++) {
        Q_set[i] = new CLUtils::MO<CL_MEM_READ_WRITE>(context, (Nx+4)*(Ny+4)*sizeof(cl_float3), NULL);
    }
    Sx_set = new CLUtils::MO<CL_MEM_READ_WRITE>(context, (Nx+4)*(Ny+4)*sizeof(cl_float3), NULL);
    Sy_set = new CLUtils::MO<CL_MEM_READ_WRITE>(context, (Nx+4)*(Ny+4)*sizeof(cl_float3), NULL);
    F_set  = new CLUtils::MO<CL_MEM_READ_WRITE>(context, (Nx+4)*(Ny+4)*sizeof(cl_float3), NULL);
    G_set  = new CLUtils::MO<CL_MEM_READ_WRITE>(context, (Nx+4)*(Ny+4)*sizeof(cl_float3), NULL);
    E_set  = new CLUtils::MO<CL_MEM_WRITE_ONLY>(context, (Nx+4)*(Ny+4)*sizeof(cl_float), NULL);
    
    // We dont need to visualize ghost cells
    R_tex  = new CLUtils::ImageBuffer<CL_MEM_READ_WRITE>(context, tex, (Nx), (Ny), NULL);
    
    CHECK_GL_ERRORS();
}

void AppManager::createTriangleStrip(unsigned int Nx, unsigned int Ny, BO<GL_ARRAY_BUFFER>*& vert, BO<GL_ELEMENT_ARRAY_BUFFER>*& ind){
    
    float dx = 1.0f/static_cast<float>(Nx);
	float dy = 1.0f/static_cast<float>(Ny);
    
    std::vector<GLfloat>    vertices;
    std::vector<GLuint>     indices;
    
	//Vertices
	vertices.reserve((Nx+1)*(Ny+1));
	for (unsigned int j=0; j<=Ny; ++j) {
		for (unsigned int i=0; i<=Nx; ++i) {
			vertices.push_back(i*dx);	//x
			vertices.push_back(j*dy);	//y
		}
	}
    vert = new BO<GL_ARRAY_BUFFER>(vertices.data(),sizeof(GLfloat)*(GLuint)vertices.size());
    
	restart_token = Nx * Ny * 2;
    
	//Indices
	for (unsigned int j=0; j<Ny; ++j) {
		for (unsigned int i=0; i<=Nx; ++i) {
			indices.push_back(    j*(Nx+1)+i);
			indices.push_back((j+1)*(Nx+1)+i);
		}
		indices.push_back(restart_token);
	}
    indices_count = (GLuint)indices.size();
    ind = new BO<GL_ELEMENT_ARRAY_BUFFER>(indices.data(),sizeof(GLfloat)*(GLuint)indices.size());
}

void AppManager::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

void AppManager::mouse_callback(GLFWwindow* window, int button, int action, int mods){
    double x,y;
    glfwGetCursorPos(window, &x, &y);
    
    if (action == GLFW_PRESS) {
        trackball.rotateBegin((int)x, (int)y);
    } else {
        trackball.rotateEnd((int)x, (int)y);
    }
}

void AppManager::cursor_callback(GLFWwindow* window, double x, double y){
    trackball.rotate(x,y);
}