#ifndef _CLUTILS_HPP__
#define _CLUTILS_HPP__

#include <cstdlib>
#include <sstream>
#include <vector>
#include <assert.h>
#include <iostream>
#include <fstream>

#include <OpenCL/opencl.h>
#include <OpenGL/OpenGL.h>

namespace CLUtils {
    
    struct CLcontext{
        cl_command_queue    queue;
        cl_device_id        device;
        cl_context          context;
        cl_platform_id      platform;
        
    };
    
    inline void printDeviceInfo(cl_device_id device){
        std::string name, vendor;
        name.resize(128);
        vendor.resize(128);
        
        size_t size;
        
        clGetDeviceInfo(device, CL_DEVICE_NAME, 128, &name[0], &size);
        name.resize(size-1);
        
        clGetDeviceInfo(device, CL_DEVICE_VENDOR, 128, &vendor[0], &size);
        vendor.resize(size-1);
        
        std::cout << vendor << " : " << name << std::endl;
    }
    
    inline void createContext(CLcontext& c){
        cl_int err = CL_SUCCESS;
        err |= clGetPlatformIDs(1, &c.platform, NULL);
        if(err != CL_SUCCESS){
            THROW_EXCEPTION("Failed to find platform ID");
        }
        
        err |= clGetDeviceIDs(c.platform, CL_DEVICE_TYPE_GPU, 1, &c.device, NULL);
        if(c.device == NULL){
            err &= clGetDeviceIDs(c.platform, CL_DEVICE_TYPE_CPU, 1, &c.device, NULL);
        }
        if(err != CL_SUCCESS){
            THROW_EXCEPTION("Failed to find device ID");
        }
        
        // Create the properties for this context.
        cl_context_properties prop[] = {
            // We need to add information about the OpenGL context with
            // which we want to exchange information with the OpenCL context
            CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE , (cl_context_properties) CGLGetShareGroup(CGLGetCurrentContext()) ,
            CL_CONTEXT_PLATFORM , (cl_context_properties) c.platform ,
            0 , 0 ,
        };
        
        c.context   = clCreateContext(prop, 1, &c.device, NULL, NULL, &err);
        if(err != CL_SUCCESS){
            THROW_EXCEPTION("Failed to create context");
        }
        
        c.queue     = clCreateCommandQueue(c.context, c.device, NULL, &err);
        if(err != CL_SUCCESS){
            THROW_EXCEPTION("Failed to initialize command queue");
        }
    }
    
}; //Namespace CLUtils

#include "MO.hpp"
#include "CLProgram.hpp"
#include "ImageBuffer.hpp"

#endif