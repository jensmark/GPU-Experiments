#ifndef _IMAGE_BUFFER_HPP__
#define _IMAGE_BUFFER_HPP__

#include <OpenCL/opencl.h>

namespace CLUtils {
    
    template <cl_mem_flags T>
    class ImageBuffer {
    public:
        ImageBuffer(CLcontext context, unsigned short components, unsigned short width, unsigned short height, size_t bytes, void* data) {
            cl_int err;
            
            cl_image_format format;
            if(components == 4)
                format.image_channel_order = CL_RGBA;
            else
                format.image_channel_order = CL_RGB;
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
        
        ImageBuffer(CLcontext context, GLuint texture){
            cl_int err;
            
            image = clCreateFromGLTexture(context.context, T, GL_TEXTURE_2D, 0, texture, &err);
            if(err != CL_SUCCESS){
                THROW_EXCEPTION("Failed to create memory object");
            }
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