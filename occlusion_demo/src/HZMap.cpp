#include "HZMap.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

HZMap::HZMap(unsigned int width, unsigned int height){
    this->width = width;
	this->height = height;

	// Initialize Texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height,
                 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glGenerateMipmap(GL_TEXTURE_2D);
    
	// Create FBO and attach buffers
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
	
	CHECK_GL_ERRORS();
	CHECK_GL_FBO_COMPLETENESS();
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    HZMbuilder = new GLUtils::Program("res/shaders/kernel.vert","res/shaders/kernel.frag","");
    quad = new ScreenQuad(HZMbuilder);
}

HZMap::~HZMap(){
    glDeleteFramebuffers(1, &fbo);
    
    delete HZMbuilder;
    delete quad;
}

void HZMap::begin(){
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture, 0);
    
    glViewport(0,0,width,height);
    
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    CHECK_GL_ERRORS();
    CHECK_GL_FBO_COMPLETENESS();
}

void HZMap::end(){
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void HZMap::build(){
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glBindTexture(GL_TEXTURE_2D, texture);
    int numLevels = 1 + (int)glm::floor(glm::log2(glm::max((float)width, (float)height)));
    
    int cWidth = width;
    int cHeight = height;
    for (int i = 1; i < numLevels; i++) {
        HZMbuilder->use();
        glUniform2iv(HZMbuilder->getUniform("lastMipSize"),1,
                     glm::value_ptr(glm::ivec2(cWidth,cHeight)));

        cWidth /= 2;
        cHeight /= 2;
        cWidth = (cWidth > 0) ? cWidth : 1;
        cHeight = (cHeight > 0) ? cHeight : 1;
        
        glViewport(0,0,cWidth,cHeight);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, i-1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, i-1);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture, i);
        
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        
        CHECK_GL_ERRORS();
        CHECK_GL_FBO_COMPLETENESS();
        
        // Render full screen quad
        quad->render(HZMbuilder);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, numLevels-1);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}