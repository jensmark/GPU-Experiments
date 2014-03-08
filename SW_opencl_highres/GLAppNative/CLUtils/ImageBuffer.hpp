#ifndef _IMAGE_BUFFER_HPP__
#define _IMAGE_BUFFER_HPP__

#include <OpenCL/opencl.h>

namespace CLUtils {
    
    template <cl_mem_flags T>
    class ImageBuffer {
    public:
        ImageBuffer(CLcontext context, unsigned short width, unsigned short height, void* data) {
            cl_int err;
            
            cl_image_format format;
            format.image_channel_order = CL_RGBA;
            format.image_channel_data_type = CL_FLOAT;
            
            cl_image_desc desc;
            desc.image_width = width;
            desc.image_height = height;
            desc.image_type = CL_MEM_OBJECT_IMAGE2D;
            
            image = clCreateImage(context.context, T, &format, &desc, data, &err);
            if(err != CL_SUCCESS){
                THROW_EXCEPTION("Failed to create memory object");
            }
        }
        
        ImageBuffer(CLcontext context, const GLuint& texture){
            cl_int err;
            
            image = clCreateFromGLTexture(context.context, T, GL_TEXTURE_2D, 0, texture, &err);
            if(err != CL_SUCCESS){
                THROW_EXCEPTION("Failed to create memory object");
            }
        }
        
        ImageBuffer(CLcontext context, GLuint& texture, unsigned short width, unsigned short height, void* data = NULL){
            
            cl_int err;
            
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, data);
            
            image = clCreateFromGLTexture(context.context, T, GL_TEXTURE_2D, 0, texture, &err);
            if(err != CL_SUCCESS){
                THROW_EXCEPTION("Failed to create memory object");
            }
            
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        
        ~ImageBuffer() {
            clReleaseMemObject(image);
        }
        
        cl_image& getRef(){
            return image;
        }
        
    private:
        ImageBuffer() {}
        cl_image image;
    };
    
};//namespace CLUtils

#endif