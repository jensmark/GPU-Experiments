#ifndef _MO_HPP__
#define _MO_HPP__

#include <OpenCL/opencl.h>

namespace CLUtils {
    
    template <cl_mem_flags T>
    class MO {
    public:
        MO(CLcontext& context, size_t bytes, void* data) {
            cl_int err;
            
            mem = clCreateBuffer(context.context, T, bytes, data, &err);
            if(err != CL_SUCCESS){
                THROW_EXCEPTION("Failed to create memory object");
            }
        }
        
        MO(CLcontext& context, GLUtils::BO<GL_ARRAY_BUFFER> buffer){
            cl_int err;
            
            mem = clCreateFromGLBuffer(context.context, T, buffer.name(), &err);
            if(err != CL_SUCCESS){
                THROW_EXCEPTION("Failed to create memory object");
            }
        }
        
        cl_mem& getRef(){
            return mem;
        }
        
        ~MO() {
            clReleaseMemObject(mem);
        }
        
    private:
        MO() {}
        cl_mem mem;
    };
    
};//namespace CLUtils

#endif