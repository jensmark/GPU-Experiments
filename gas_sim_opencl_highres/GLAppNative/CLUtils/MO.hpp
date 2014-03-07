#ifndef _MO_HPP__
#define _MO_HPP__

#include <OpenCL/opencl.h>

namespace CLUtils {
    
    template <cl_mem_flags T>
    class MO {
    public:
        MO(CLcontext& context, size_t bytes, void* data) {
            cl_int err;
            
            this->bytes = bytes;
            this->context = &context;
            
            mem = clCreateBuffer(context.context, T, bytes, data, &err);
            if(err != CL_SUCCESS){
                THROW_EXCEPTION("Failed to create memory object");
            }
        }
        
        MO(CLcontext& context, GLUtils::BO<GL_ARRAY_BUFFER> buffer){
            cl_int err;
            
            this->bytes = bytes;
            this->context = &context;
            
            mem = clCreateFromGLBuffer(context.context, T, buffer.name(), &err);
            if(err != CL_SUCCESS){
                THROW_EXCEPTION("Failed to create memory object");
            }
        }
        
        void upload(void* data){
            clEnqueueWriteBuffer(context->queue, mem, CL_TRUE, 0, bytes, data, 0, NULL, NULL);
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
        CLcontext* context;
        size_t bytes;
    };
    
};//namespace CLUtils

#endif