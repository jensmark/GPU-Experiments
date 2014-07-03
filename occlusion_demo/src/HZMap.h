#ifndef _HZMap_h
#define _HZMap_h

#include "GLUtils.hpp"
#include "ScreenQuad.h"

class HZMap {
public:
    HZMap(unsigned int width, unsigned int height);
    ~HZMap();

    /**
     * Start rendering occluders
     */
    void begin();
    
    /**
     * End rendering occluders
     */
    void end();
    
    /**
     * Build hirarchical Z-map from rendering results
     */
    void build();
    
    /**
     * Return with and height of the fbo
     */
    unsigned int getWidth() {return width;}
	unsigned int getHeight() {return height;}
    
    /**
     * Return the final mipmapped texture
     */
    GLuint getTexture() {return texture;}
    
private:
    GLuint fbo;
    GLuint texture;
	unsigned int width, height;
    
    ScreenQuad* quad;
    GLUtils::Program* HZMbuilder;
};

#endif
