//
//  ScreenQuad.h
//  
//
//  Created by Jens Kristoffer Reitan Markussen on 25.06.14.
//
//

#ifndef ____ScreenQuad__
#define ____ScreenQuad__

#include "GLUtils.hpp"

#include <iostream>

class ScreenQuad {
public:
    ScreenQuad(GLUtils::Program* shader);
    ~ScreenQuad();

    void render(GLUtils::Program* shader);

private:
    GLuint vao; // < Vertex array object for render
    
    // Buffer objects
    GLUtils::BO<GL_ARRAY_BUFFER>* vert;
    GLUtils::BO<GL_ELEMENT_ARRAY_BUFFER>* ind;
};

#endif /* defined(____ScreenQuad__) */
