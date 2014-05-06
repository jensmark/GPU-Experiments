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

#include "CLUtils.hpp"

#include "Visualizer.h"
#include "SimulatorBase.h"

#include <vector>

using namespace GLUtils;

enum Solver{
    UNKNOWN_SOLVER, GL_EULER, CL_EULER, CL_SW
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
	void init(size_t Nx, size_t Ny, Solver type, const char* dev);
    
	/**
	 * The main loop of the app. Runs the main loop
	 */
	void begin(size_t N, float T);
    
private:
    /**
	 * Quit function
	 */
	void quit();
    
    /**
     * Write results to json
     */
    void writeJSON();
    
private:
    static const unsigned int window_width  = 800;
	static const unsigned int window_height = 600;
    
    Visualizer*         visualizer;
    SimulatorBase*      simulator;
    
    Solver type;
    std::string prefix;
    
    struct{
        float total_sim_time;
        float average_timestep;
        size_t N;
        size_t Nx;
        size_t Ny;
        float time;
        float max_sim_time;
        float min_sim_time;
    }results;
};

#endif
