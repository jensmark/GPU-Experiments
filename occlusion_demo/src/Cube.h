#ifndef _CUBE__
#define _CUBE__

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "GLUtils.hpp"

class Cube {
public:
	Cube(GLUtils::Program* shader, glm::vec3 position, glm::quat rotation, glm::vec3 scale);
	~Cube();

    /**
     * Return the model matrix
     */
    glm::mat4& getMatrix();
    
    /**
     * Render the cubes
     */
    void render(GLUtils::Program* shader, glm::mat4& proj_matrix, glm::mat4& view_matrix);
    
private:
    glm::mat4 model; // < Model matrix
    GLuint vao; // < Vertex array object for render
    
    // Buffer objects
    GLUtils::BO<GL_ARRAY_BUFFER>* vert;
    GLUtils::BO<GL_ARRAY_BUFFER>* norm;
};

#endif