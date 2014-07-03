//
//  ScreenQuad.cpp
//  
//
//  Created by Jens Kristoffer Reitan Markussen on 25.06.14.
//
//

#include "ScreenQuad.h"


ScreenQuad::ScreenQuad(GLUtils::Program* shader){
    glGenVertexArrays(1,&vao);
    glBindVertexArray(vao);
    GLfloat quad_vertices[] =  {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        1.0f,  1.0f,
        -1.0f,  1.0f,
    };
    vert = new GLUtils::BO<GL_ARRAY_BUFFER>(quad_vertices, sizeof(quad_vertices));
    
    GLubyte quad_indices[] = {
        0, 1, 2, //triangle 1
        2, 3, 0, //triangle 2
    };
    ind = new GLUtils::BO<GL_ELEMENT_ARRAY_BUFFER>(quad_indices, sizeof(quad_indices));
    vert->bind();
    shader->setAttributePointer("position", 2);
    ind->bind();
    glBindVertexArray(0);
}

ScreenQuad::~ScreenQuad(){
    delete vert;
    delete ind;
    
    glDeleteVertexArrays(1, &vao);
}

void ScreenQuad::render(GLUtils::Program* shader){
    shader->use();
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_BYTE,NULL);
    glBindVertexArray(0);
    shader->disuse();
}