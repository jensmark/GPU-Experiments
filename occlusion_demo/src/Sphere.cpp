#include "Sphere.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>

Sphere::Sphere(GLUtils::Program* shader, unsigned int tesselation, float scale, glm::vec3 position){
    
    model = glm::scale(glm::mat4(1.0f), glm::vec3(scale));
    model = glm::translate(model, position);
    
    float a = 1.0f/static_cast<float>(tesselation);
	float b = 1.0f/static_cast<float>(tesselation-1);
    
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<unsigned int> indices;
    
    positions.resize(3*4*(tesselation-1)*tesselation);
    normals.resize(3*4*(tesselation-1)*tesselation);
	indices.resize(3*2*(tesselation-1)*tesselation);
    
    float* p = &positions[0];
    float* n = &normals[0];
    unsigned int* x = &indices[0];
    unsigned int k = 0;
    
    //Create a vector of vertices for use with indexed rendering
	for (unsigned int i=0; i<tesselation-1; i++) {
		for (unsigned int j=0; j<tesselation; j++) {
			// Create the four vertices in the "quad"
			evaluateAt(a*j, b*i, p, n);
			p+=3;
            n+=3;
			evaluateAt(a*(j+1), b*i, p, n);
			p+=3;
            n+=3;
			evaluateAt(a*j, b*(i+1), p, n);
			p+=3;
            n+=3;
			evaluateAt(a*(j+1), b*(i+1), p, n);
			p+=3;
            n+=3;
            
			//Create the indices for the upper left triangle
			x[0] = k;
			x[1] = k+1;
			x[2] = k+2;
			x+=3;
			
			//Create the indices for the lower right triangle
			x[0] = k+1;
			x[1] = k+3;
			x[2] = k+2;
			x+=3;
            
			k += 4;
		}
	}
    
    vert = new GLUtils::BO<GL_ARRAY_BUFFER>(
                                positions.data(),
                                sizeof(float)*(uint)positions.size());
    norm = new GLUtils::BO<GL_ARRAY_BUFFER>(
                                normals.data(),
                                sizeof(float)*(uint)normals.size());
    ind = new GLUtils::BO<GL_ELEMENT_ARRAY_BUFFER>(
                                indices.data(),
                                sizeof(unsigned int)*(uint)indices.size());
    ind_count = (uint)indices.size();
    
    
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    vert->bind();
	shader->setAttributePointer("position", 3);
    
    norm->bind();
    shader->setAttributePointer("normal", 3);
    
    vert->unbind();
    ind->bind();
	glBindVertexArray(0);
}
Sphere::~Sphere(){
    delete vert;
    delete norm;
    delete ind;
    
    glDeleteVertexArrays(1, &vao);
}

glm::mat4& Sphere::getMatrix(){
    return model;
}

void Sphere::render(GLUtils::Program* shader, glm::mat4& proj_matrix, glm::mat4& view_matrix){
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
    glDrawElements(GL_TRIANGLES, ind_count, GL_UNSIGNED_INT, NULL);
    glBindVertexArray(0);
    
    shader->disuse();
}

void Sphere::evaluateAt(float u, float v, float* pos, float* nor){
    static const double PI = 3.14159265;
    
	pos[0] = (float)(std::sin(u*2*PI) * std::cos(v*2*PI));
	pos[1] = (float)(std::sin(u*2*PI) * std::sin(v*2*PI));
	pos[2] = (float)(std::cos(u*2*PI));
    
    nor[0] = pos[0];
	nor[1] = pos[1];
	nor[2] = pos[2];
}