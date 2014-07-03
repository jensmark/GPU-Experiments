#ifndef _SPHERE__
#define _SPHERE__

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "GLUtils.hpp"

class Sphere {
public:
	Sphere(GLUtils::Program* shader, unsigned int tesselation, float scale, glm::vec3 position);
	~Sphere();

    /**
     * Return the model matrix
     */
    glm::mat4& getMatrix();
    
    /**
     * Render the shpere
     */
    void render(GLUtils::Program* shader, glm::mat4& proj_matrix, glm::mat4& view_matrix);

private:
	/**
     * Evaluate spherical coordinate
     */
    void evaluateAt(float u, float v, float* pos, float* nor);
    
private:
    glm::mat4 model; // < Model matrix
    GLuint vao; // < Vertex array object for render
    GLuint ind_count; // < indices count
    
    // Buffer objects
    GLUtils::BO<GL_ARRAY_BUFFER>* vert;
    GLUtils::BO<GL_ARRAY_BUFFER>* norm;
    GLUtils::BO<GL_ELEMENT_ARRAY_BUFFER>* ind;
};

#endif