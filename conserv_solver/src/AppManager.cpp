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

#include "SimulatorCLSW.h"
#include "SimulatorGLEuler.h"
#include "SimulatorCLEuler.h"


AppManager::AppManager(){
}

AppManager::~AppManager(){
}

void AppManager::init(size_t Nx, size_t Ny, Solver type, const char* dev){
    std::cout << "Initializing simulating parameters" << std::endl;
    
    visualizer  = new Visualizer(TEXTURE,window_width,window_height);
    visualizer->init();
    
    this->type = type;
    
    cl_device_type dev_type;
    if (dev == NULL) {
        dev_type = CL_DEVICE_TYPE_GPU;
        prefix = "GPU_";
    } else if (std::string(dev).compare("CPU") == 0) {
        prefix = "CPU_";
        dev_type = CL_DEVICE_TYPE_CPU;
    } else if (std::string(dev).compare("GPU") == 0) {
        dev_type = CL_DEVICE_TYPE_GPU;
        prefix = "GPU_";
    }
    
    switch (type) {
        case GL_EULER:
            simulator   = new SimulatorGLEuler();
            break;
        case CL_EULER:
            simulator   = new SimulatorCLEuler(dev_type);
            break;
        case CL_SW:
            simulator   = new SimulatorCLSW(dev_type);
            break;
        default:
            THROW_EXCEPTION("Unknown solver");
            break;
    }
    
    simulator->init(Nx,Ny,"");
    
    visualizer->setSimulator(simulator);
    
    results.Nx = Nx;
    results.Ny = Ny;
    
    results.max_sim_time = -std::numeric_limits<float>().max();
    results.min_sim_time =  std::numeric_limits<float>().max();
}

void AppManager::begin(size_t N, float T){
    std::cout << "Simulation starting with [" <<
        results.Nx << "x" << results.Ny << "] grid" << std::endl;
    
    results.total_sim_time = 0;
    size_t c = 0;
    
    while (!glfwWindowShouldClose(visualizer->getWindow()) && c < N) {
        /* Poll for and process events */
        glfwPollEvents();
        
        SimDetail details = simulator->simulate();
        visualizer->render();
        
        results.total_sim_time += details.sim_time;
        results.time = details.time;
        
        results.max_sim_time = glm::max(results.max_sim_time, (float)details.sim_time);
        results.min_sim_time = glm::min(results.min_sim_time, (float)details.sim_time);
        
        /* Swap front and back buffers */
        glfwSwapBuffers(visualizer->getWindow());
        
        c++;
        
        if (details.time > T) {
            break;
        }
    }
    
    results.average_timestep = results.total_sim_time/c;
    results.N = c;
    
    writeJSON();
    
    /* Clean up everything */
    quit();
}

void AppManager::quit(){
    delete simulator;
    delete visualizer;
}

void AppManager::writeJSON(){
    std::ofstream output;
    std::stringstream file;
    
    std::string str;
    switch (this->type) {
        case GL_EULER:
            str   = "GLEULER_";
            break;
        case CL_EULER:
            str   = "CLEULER_";
            break;
        case CL_SW:
            str   = "CLSW_";
            break;
        default:
            THROW_EXCEPTION("Unknown solver");
            break;
    }
    
    file << prefix << str << results.Nx << "x" << results.Ny << ".json";
    
    std::cout << "Saving simulation details as: " << file.str() << std::endl;
    
    output.open(file.str());
    output  << "{" << std::endl;
    
    output  << "\t\"total_sim_time\":" << results.total_sim_time << "," << std::endl;
    output  << "\t\"average_timestep\":" << results.average_timestep << "," << std::endl;
    output  << "\t\"max_timestep\":" << results.max_sim_time << "," << std::endl;
    output  << "\t\"min_timestep\":" << results.min_sim_time << "," << std::endl;
    output  << "\t\"N\":" << results.N << "," << std::endl;
    output  << "\t\"Nx\":" << results.Nx << "," << std::endl;
    output  << "\t\"Ny\":" << results.Ny << "," << std::endl;
    output  << "\t\"time\":" << results.time << std::endl;

    output  << "}";
    output.close();
}

