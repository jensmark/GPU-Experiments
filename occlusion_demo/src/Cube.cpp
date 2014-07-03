#include "Cube.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>

Cube::Cube(GLUtils::Program* shader, glm::vec3 position, glm::quat rotation, glm::vec3 scale){
    
    model = glm::mat4_cast(rotation);
    model = glm::scale(model, scale);
    model = glm::translate(model, position);
    
    float vertex_data[] = {
        -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f,
        0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f,
        0.5f, -0.5f, 0.5f,
        0.5f, 0.5f, 0.5f,
        
        0.5f, 0.5f, 0.5f,
        0.5f, -0.5f, 0.5f,
        0.5f, 0.5f, -0.5f,
        0.5f, -0.5f, 0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, 0.5f, -0.5f,
        
        0.5f, 0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        -0.5f, 0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, 0.5f, -0.5f,
        
        -0.5f, 0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, 0.5f,
        -0.5f, 0.5f, 0.5f,
        
        -0.5f, 0.5f, 0.5f,
        0.5f, 0.5f, 0.5f,
        -0.5f, 0.5f, -0.5f,
        0.5f, 0.5f, 0.5f,
        0.5f, 0.5f, -0.5f,
        -0.5f, 0.5f, -0.5f,
        
        0.5f, -0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f,
        0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, 0.5f,
        -0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f
    };
    
    static const float normal_data[] = {
        0.0f, 0.0f, 1,
        0.0f, 0.0f, 1,
        0.0f, 0.0f, 1,
        0.0f, 0.0f, 1,
        0.0f, 0.0f, 1,
        0.0f, 0.0f, 1,
        
        1, 0.0f, 0.0f,
        1, 0.0f, 0.0f,
        1, 0.0f, 0.0f,
        1, 0.0f, 0.0f,
        1, 0.0f, 0.0f,
        1, 0.0f, 0.0f,
        
        0.0f, 0.0f, -1,
        0.0f, 0.0f, -1,
        0.0f, 0.0f, -1,
        0.0f, 0.0f, -1,
        0.0f, 0.0f, -1,
        0.0f, 0.0f, -1,
        
        -1, 0.0f, 0.0f,
        -1, 0.0f, 0.0f,
        -1, 0.0f, 0.0f,
        -1, 0.0f, 0.0f,
        -1, 0.0f, 0.0f,
        -1, 0.0f, 0.0f,
        
        0.0f, 1, 0.0f,
        0.0f, 1, 0.0f,
        0.0f, 1, 0.0f,
        0.0f, 1, 0.0f,
        0.0f, 1, 0.0f,
        0.0f, 1, 0.0f,
        
        0.0f, -1, 0.0f,
        0.0f, -1, 0.0f,
        0.0f, -1, 0.0f,
        0.0f, -1, 0.0f,
        0.0f, -1, 0.0f,
        0.0f, -1, 0.0f,
    };
    
    
    vert = new GLUtils::BO<GL_ARRAY_BUFFER>(
                                &vertex_data[0],
                                sizeof(vertex_data));
    norm = new GLUtils::BO<GL_ARRAY_BUFFER>(
                                &normal_data[0],
                                sizeof(normal_data));
    
    
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    vert->bind();
	shader->setAttributePointer("position", 3);
    
    norm->bind();
    shader->setAttributePointer("normal", 3);
    
    vert->unbind();
	glBindVertexArray(0);
}
Cube::~Cube(){
    delete vert;
    delete norm;
    
    glDeleteVertexArrays(1, &vao);
}

glm::mat4& Cube::getMatrix(){
    return model;
}

void Cube::render(GLUtils::Program* shader, glm::mat4& proj_matrix, glm::mat4& view_matrix){
    shader->use();
    
    glm::mat4 modelview = view_matrix*model;
    glm::mat3 normal    = glm::mat3(glm::inverse(glm::transpose(modelview)));
    
    glUniformMatrix4fv(
        shader->getUniform("proj_matrix"), 1, GL_FALSE, glm::value_ptr(proj_matrix)
    );
    glUniformMatrix4fv(
        shader->getUniform("modelview_matrix"), 1, GL_FALSE, glm::value_ptr(modelview)
    );
    glUniformMatrix3fv(
        shader->getUniform("normal_matrix"), 1, GL_FALSE, glm::value_ptr(normal)
    );
    
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    
    shader->disuse();
}
