//
//  SimulatorBase
//  GLAppNative
//
//  Created by Jens Kristoffer Reitan Markussen on 28.12.13.
//  Copyright (c) 2013 Jens Kristoffer Reitan Markussen. All rights reserved.
//

#ifndef GLAppNative_SimulatorBase_h
#define GLAppNative_SimulatorBase_h

#include <string>
#include <vector>
#include <glm/glm.hpp>

struct SimDetail{
    double sim_time;
    float time;
    float dt;
};

class SimulatorBase{
public:
    /**
	 * Constructor
	 */
	SimulatorBase(){};
    
	/**
	 * Destructor
	 */
	virtual ~SimulatorBase(){};
    
	/**
	 * Initializes the simulator
	 */
	virtual void init(size_t Nx, size_t Ny, std::string initialKernel) = 0;
    
    /**
     * Run one step of the simulator
     */
    virtual SimDetail simulate() = 0;
    
    /**
     * Get the data as opengl texture
     */
    virtual size_t getTexture() = 0;
    
    /**
     * Get the data as std vector
     */
    virtual std::vector<float> getData() = 0;
    
    /**
     * Return the size of the grid
     */
    virtual glm::ivec2 getGridSize() = 0;
    
    /**
     * Return grid delta x and y
     */
    virtual glm::vec2 getDeltaXY() = 0;
    
    /**
     * Get current sim time
     */
    virtual float getTime() = 0;
};

#endif
