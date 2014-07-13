//
//  AppManager.cpp
//  GLAppNative
//
//  Created by Jens Kristoffer Reitan Markussen on 28.12.13.
//  Copyright (c) 2013 Jens Kristoffer Reitan Markussen. All rights reserved.
//

#include "AppManager.h"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>
#include <glm/gtx/string_cast.hpp>

// Static decleration
VirtualTrackball AppManager::trackball;
float AppManager::fovx = 65.0f;
RenderMode AppManager::mode = PHONG;

AppManager::AppManager(){
}

AppManager::~AppManager(){
}

void AppManager::init(std::string model){
    /* Initialize the library */
    if (glfwInit() != GL_TRUE) {
        THROW_EXCEPTION("Failed to initialize GLFW");
    }
    glfwSetErrorCallback(error_callback);
    
    std::stringstream ss;
    ss << "res/models/" << model << ".obj";
    filepath = ss.str();
    
    createOpenGLContext();
    
    ilInit();
    iluInit();
    ilOriginFunc(IL_ORIGIN_LOWER_LEFT);
	ilEnable(IL_ORIGIN_SET);
    
    camera.projection = glm::perspective(fovx,
                                         window_width / (float) window_height, 1.0f, 10.0f);
	camera.view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5.0f));
    model_matrix = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));
    
    trackball.setWindowSize(window_width, window_height);
    
    setOpenGLStates();
    createProgram();
    createVAO();
    
    int width, height;
    glfwGetFramebufferSize(window,&width,&height);
    deferred_kernel.reset(new TextureFBO(width, height, 6, true));
}

void AppManager::begin(){
    
    while (!glfwWindowShouldClose(window)) {
        /* Poll for and process events */
        glfwPollEvents();
        
        render();
        
        /* Swap front and back buffers */
        glfwSwapBuffers(window);
    }
    
    /* Clean up everything */
    quit();
}

void AppManager::renderMeshRecursive(MeshPart& mesh, const std::shared_ptr<Program>& program, const glm::mat4& view_matrix, const glm::mat4& model_matrix) {
	//Create modelview matrix
	glm::mat4 meshpart_model_matrix = model_matrix*mesh.transform;
	glm::mat4 modelview_matrix = view_matrix*meshpart_model_matrix;
	glUniformMatrix4fv(program->getUniform("modelview_matrix"), 1, 0, glm::value_ptr(modelview_matrix));

	//Create normal matrix, the transpose of the inverse
	//3x3 leading submatrix of the modelview matrix
	glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(modelview_matrix)));
	glUniformMatrix3fv(program->getUniform("normal_matrix"), 1, 0, glm::value_ptr(normal_matrix));
    
	// Set materiale for this mesh
	glUniform3f(program->getUniform("diffuse"),
                mesh.materiale.diffuse.r,
                mesh.materiale.diffuse.g,
                mesh.materiale.diffuse.b);
    
	glUniform3f(program->getUniform("specular"),
                mesh.materiale.specular.r,
                mesh.materiale.specular.g,
                mesh.materiale.specular.b);
    
	glUniform3f(program->getUniform("ambient"),
                mesh.materiale.ambient.r,
                mesh.materiale.ambient.g,
                mesh.materiale.ambient.b);
    
	glUniform1f(program->getUniform("shininess"),
                mesh.materiale.shininess);
    
    glUniform1i(program->getUniform("main_tex"),0);
    glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mesh.materiale.textureID);
    
	// Render mesh
	glDrawElements(GL_TRIANGLES, mesh.count, GL_UNSIGNED_INT,  (void*)(sizeof(unsigned int) * mesh.first));
	
	glBindTexture(GL_TEXTURE_2D, 0);
    
    CHECK_GL_ERRORS();
    
	for (unsigned int i=0; i<mesh.children.size(); ++i)
		renderMeshRecursive(mesh.children.at(i), program, view_matrix, meshpart_model_matrix);
}

void AppManager::render(){
    glm::mat4 view_matrix_new = camera.view*trackball.getTransform();
    
    deferred->use();
    
    camera.projection = glm::perspective(fovx,
                window_width / (float) window_height, 1.0f, 10.0f);
    
    glUniformMatrix4fv(deferred->getUniform("projection_matrix"), 1, 0, glm::value_ptr(camera.projection));
    
    deferred_kernel->bind();
    glViewport(0, 0, deferred_kernel->getWidth(), deferred_kernel->getHeight());
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
	//Render geometry
	glBindVertexArray(vao[0]);
	renderMeshRecursive(model->getMesh(), deferred, view_matrix_new, model_matrix);
	glBindVertexArray(0);
    deferred_kernel->unbind();
    
    deferred->disuse();
    
    int width, height;
    glfwGetFramebufferSize(window,&width,&height);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    switch (mode) {
        case PHONG:
            renderPhong();
            break;
        case GOOCH:
            renderGooch();
            break;
        case GOOCH_OUTLINE:
            renderGoochOutline();
            break;
    }
    
    glFinish();
    CHECK_GL_ERRORS();
}

void AppManager::renderPhong(){
    phong->use();
    
    glUniform1i(phong->getUniform("ambient_map"),0);
    glUniform1i(phong->getUniform("diffuse_map"),1);
    glUniform1i(phong->getUniform("specular_map"),2);
    glUniform1i(phong->getUniform("normal_map"),3);
    glUniform1i(phong->getUniform("light_map"),5);
    
    for (size_t i = 0; i < 6; i++) {
        glActiveTexture(GL_TEXTURE0+i);
        glBindTexture(GL_TEXTURE_2D,deferred_kernel->getTexture(i));
    }
    
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D,deferred_kernel->getDepth());
    
    glBindVertexArray(vao[1]);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE,  NULL);
	glBindVertexArray(0);
    
    phong->disuse();
}

void AppManager::renderGooch(){
    gooch->use();
    
    glUniform1i(gooch->getUniform("ambient_map"),0);
    glUniform1i(gooch->getUniform("diffuse_map"),1);
    glUniform1i(gooch->getUniform("specular_map"),2);
    glUniform1i(gooch->getUniform("normal_map"),3);
    glUniform1i(gooch->getUniform("light_map"),5);
    
    for (size_t i = 0; i < 6; i++) {
        glActiveTexture(GL_TEXTURE0+i);
        glBindTexture(GL_TEXTURE_2D,deferred_kernel->getTexture(i));
    }
    
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D,deferred_kernel->getDepth());
    
    glBindVertexArray(vao[1]);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE,  NULL);
	glBindVertexArray(0);
    
    gooch->disuse();
}

void AppManager::renderGoochOutline(){
    gooch_outline->use();
    
    glUniform1i(gooch_outline->getUniform("ambient_map"),0);
    glUniform1i(gooch_outline->getUniform("diffuse_map"),1);
    glUniform1i(gooch_outline->getUniform("specular_map"),2);
    glUniform1i(gooch_outline->getUniform("normal_map"),3);
    glUniform1i(gooch_outline->getUniform("light_map"),5);
    glUniform1i(gooch_outline->getUniform("depth_map"),6);
    
    for (size_t i = 0; i < 6; i++) {
        glActiveTexture(GL_TEXTURE0+i);
        glBindTexture(GL_TEXTURE_2D,deferred_kernel->getTexture(i));
    }
    
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D,deferred_kernel->getDepth());
    
    glBindVertexArray(vao[1]);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE,  NULL);
	glBindVertexArray(0);
    
    gooch_outline->disuse();
}

void AppManager::quit(){
    glfwDestroyWindow(window);
    glfwTerminate();
}

void AppManager::createOpenGLContext(){
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    glfwWindowHint(GLFW_DEPTH_BITS, 32);
    glfwWindowHint(GLFW_RED_BITS, 8);
    glfwWindowHint(GLFW_GREEN_BITS, 8);
    glfwWindowHint(GLFW_BLUE_BITS, 8);
    glfwWindowHint(GLFW_ALPHA_BITS, 8);
    
    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(window_width, window_height, "GL App", NULL, NULL);
    if (window == NULL) {
        THROW_EXCEPTION("Failed to create window");
    }
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_callback);
    glfwSetCursorPosCallback(window, cursor_callback);
    glfwSetScrollCallback(window,scrollwheel_callback);
    
    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    
    glewExperimental = GL_TRUE;
	GLenum glewErr = glewInit();
	if (glewErr != GLEW_OK) {
		std::stringstream err;
		err << "Error initializing GLEW: " << glewGetErrorString(glewErr);
		THROW_EXCEPTION(err.str());
	}
    
	// Unfortunately glewInit generates an OpenGL error, but does what it's
	// supposed to (setting function pointers for core functionality).
	// Lets do the ugly thing of swallowing the error....
    glGetError();
    
}
void AppManager::setOpenGLStates(){
    glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
    
    glViewport(0, 0, window_width, window_height);
	glClearColor(0.75, 0.75, 0.75, 1.0);
    
    CHECK_GL_ERRORS();
}
void AppManager::createProgram(){
    deferred.reset(new GLUtils::Program("res/shaders/transform.vert","res/shaders/deferred.frag"));
    phong.reset(new GLUtils::Program("res/shaders/kernel.vert","res/shaders/phong.frag"));
    gooch.reset(new GLUtils::Program("res/shaders/kernel.vert","res/shaders/gooch.frag"));
    gooch_outline.reset(new GLUtils::Program("res/shaders/kernel.vert","res/shaders/gooch_outline.frag"));
    debug.reset(new GLUtils::Program("res/shaders/kernel.vert","res/shaders/debug.frag"));
    
    CHECK_GL_ERRORS();
}

void AppManager::createVAO(){
    glGenVertexArrays(2, &vao[0]);
	glBindVertexArray(vao[0]);
    
	model.reset(new Model(filepath, false));
	model->getVertices()->bind();
	deferred->setAttributePointer("position", 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8);
    
	deferred->setAttributePointer("normal", 3, GL_FLOAT, GL_FALSE, (sizeof(float) * 8), (void*)(sizeof(float) * 3));
    
	deferred->setAttributePointer("uv", 2, GL_FLOAT, GL_FALSE, (sizeof(float) * 8), (void*)(sizeof(float) * 6));
    
	//Unbind VBOs and VAO
	model->getVertices()->unbind(); //Unbinds both vertices and normals
    
	model->getIndices()->bind();
    
	glBindVertexArray(0);
    
    glBindVertexArray(vao[1]);
    GLfloat quad_vertices[] =  {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        1.0f,  1.0f,
        -1.0f,  1.0f,
    };
    vert.reset(new BO<GL_ARRAY_BUFFER>(quad_vertices, sizeof(quad_vertices)));
    
    GLubyte quad_indices[] = {
        0, 1, 2, //triangle 1
        2, 3, 0, //triangle 2
    };
    ind.reset(new BO<GL_ELEMENT_ARRAY_BUFFER>(quad_indices, sizeof(quad_indices)));
    vert->bind();
    phong->setAttributePointer("position", 2);
    ind->bind();
    
    glBindVertexArray(0);
    
    
    CHECK_GL_ERRORS();
}


void AppManager::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    
    if (key == GLFW_KEY_1 && action == GLFW_PRESS)
        mode = PHONG;
    if (key == GLFW_KEY_2 && action == GLFW_PRESS)
        mode = GOOCH;
    if (key == GLFW_KEY_3 && action == GLFW_PRESS)
        mode = GOOCH_OUTLINE;
}

void AppManager::mouse_callback(GLFWwindow* window, int button, int action, int mods){
    double x,y;
    glfwGetCursorPos(window, &x, &y);
    
    if (action == GLFW_PRESS) {
        trackball.rotateBegin((int)x, (int)y);
    } else {
        trackball.rotateEnd((int)x, (int)y);
    }
}

void AppManager::cursor_callback(GLFWwindow* window, double x, double y){
    trackball.rotate(x,y);
}

void AppManager::scrollwheel_callback(GLFWwindow* window, double xoffset, double yoffset){
    fovx += (float)yoffset*0.1f;
}

