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

// Static decleration
VirtualTrackball AppManager::trackball;

AppManager::AppManager(){
}

AppManager::~AppManager(){
}

void AppManager::init(){
    /* Initialize the library */
    if (glfwInit() != GL_TRUE) {
        THROW_EXCEPTION("Failed to initialize GLFW");
    }
    glfwSetErrorCallback(error_callback);
    
    createOpenGLContext();
    
    ilInit();
    iluInit();
    
    camera.projection = glm::perspective(45.0f,
                                         window_width / (float) window_height, 1.0f, 180.0f);
	camera.view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -150.0f));
    
    trackball.setWindowSize(window_width, window_height);
    
    setOpenGLStates();
    createProgram();
    
    srand((unsigned)time(0));
    
    
    std::vector<glm::vec3> centers;
    size_t grid_size = 20;
    for (size_t x = 0; x <= grid_size; x++) {
        if (x == grid_size/2)
            continue;
        for (size_t y = 0; y <= grid_size; y++) {
            if (y == grid_size/2)
                continue;
            for (size_t z = 0; z <= grid_size; z++) {
                if (z == grid_size/2)
                    continue;
                
                float spacing = 0.3f;
                
                glm::vec3 position = glm::vec3(
                            ((float)x/spacing)-((float)grid_size/2.0f)/spacing,
                            ((float)y/spacing)-((float)grid_size/2.0f)/spacing,
                           -((float)z/spacing)+((float)grid_size/2.0f)/spacing);
                centers.push_back(position);
                
                Sphere* model = new Sphere(phong, 8, 1.0, position);
                spheres.push_back(model);
            }
        }
    }
    
    planes.push_back(
            new Cube(phong, glm::vec3(0.0f), glm::quat(), glm::vec3(80.0f, 80.0f, 1.0f))
    );
    planes.push_back(
            new Cube(phong, glm::vec3(0.0f), glm::quat(), glm::vec3(80.0f, 1.0f, 80.0f))
    );
    planes.push_back(
            new Cube(phong, glm::vec3(0.0f), glm::quat(), glm::vec3(1.0f, 80.0f, 80.0f))
    );

    hzmap = new HZMap(window_width, window_height);
    
    debug_quad = new ScreenQuad(debug);
    
    // Build bb centers geometry
    bbCenters = new GLUtils::BO<GL_ARRAY_BUFFER>(centers.data(),
                                                 sizeof(glm::vec3)*centers.size());
    bbSize = centers.size();
    glGenVertexArrays(1, &bbVao);
    glBindVertexArray(bbVao);
    
    bbCenters->bind();
	cull->setAttributePointer("position", 3);
    
    bbCenters->unbind();
	glBindVertexArray(0);
    
    // create feedback buffer
    transFeedback = new GLUtils::BO<GL_ARRAY_BUFFER>(NULL,sizeof(int)*centers.size(),GL_STATIC_READ);
    feedback.resize(bbSize);
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

void AppManager::render(){
    glm::mat4 view_matrix = camera.view*trackball.getTransform();
  
    checkOcclusion(view_matrix);
    
    // Find actual screen width and height
    int width, height;
    glfwGetFramebufferSize(window,&width,&height);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    //glBindTexture(GL_TEXTURE_2D, hzmap->getTexture());
    //debug_quad->render(debug);
    //return;
    
    // Render scene
    phong->use();
    glUniform3fv(phong->getUniform("ambient"),1,glm::value_ptr(glm::vec3(1.0f,0.0f,0.0f)));
    for (size_t i = 0; i < spheres.size(); i++) {
        if (feedback[i] == 1) {
            spheres.at(i)->render(phong, camera.projection, view_matrix);
        }
    }
    
    
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    phong->use();
    glUniform3fv(phong->getUniform("ambient"),1,glm::value_ptr(glm::vec3(0.0f,0.0f,1.0f)));
    //glUniform1f(phong->getUniform("transparency"),0.5f);
    //glCullFace(GL_BACK);
    for (size_t i = 0; i < planes.size(); i++) {
        planes.at(i)->render(phong, camera.projection, view_matrix);
    }
    //glCullFace(GL_FRONT);
    //for (size_t i = 0; i < planes.size(); i++) {
    //    planes.at(i)->render(phong, camera.projection, view_matrix);
    //}
    //glCullFace(GL_BACK);
    //glDisable(GL_BLEND);
    
    
    glFinish();
    CHECK_GL_ERRORS();
}

void AppManager::checkOcclusion(glm::mat4& view_matrix){
    // "Render" occluders
    hzmap->begin();
    for (size_t i = 0; i < planes.size(); i++) {
        planes.at(i)->render(phong, camera.projection, view_matrix);
    }
    hzmap->end();
    
    // Build hirarcial Z-map
    hzmap->build();

    glViewport(0, 0, window_width, window_height);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    glEnable(GL_RASTERIZER_DISCARD);
    
    cull->use();
    
    glUniform2f(cull->getUniform("viewport"),(float)window_width,(float)window_height);
    glUniformMatrix4fv(
            cull->getUniform("proj_matrix"), 1, GL_FALSE, glm::value_ptr(camera.projection)
    );
    glUniformMatrix4fv(
            cull->getUniform("view_matrix"), 1, GL_FALSE, glm::value_ptr(view_matrix)
    );
    
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, transFeedback->name());
    glBeginTransformFeedback(GL_POINTS);
    glBindVertexArray(bbVao);
    glDrawArrays(GL_POINTS,0,bbSize);
    glBindVertexArray(0);
    glEndTransformFeedback();

    cull->disuse();
    
    glDisable(GL_RASTERIZER_DISCARD);
    
    glFlush();
    glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, sizeof(int)*bbSize, feedback.data());
}

void AppManager::quit(){
    for (size_t i = 0; i < spheres.size(); i++) {
        delete spheres.at(i);
    }
    for (size_t i = 0; i < planes.size(); i++) {
        delete planes.at(i);
    }
    delete phong;
    delete simple;
    delete debug;
    
    delete hzmap;
    
    delete bbCenters;
    delete transFeedback;

    glDeleteVertexArrays(1, &bbVao);
    
    glfwDestroyWindow(window);
    glfwTerminate();
}

void AppManager::createOpenGLContext(){
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
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
	glClearColor(1.0, 1.0, 1.0, 1.0);
    
    CHECK_GL_ERRORS();
}
void AppManager::createProgram(){
    phong = new GLUtils::Program("res/shaders/phong.vert",
                                 "res/shaders/phong.frag","");
    simple = new GLUtils::Program("res/shaders/simple.vert",
                                  "res/shaders/simple.frag","");
    debug = new GLUtils::Program("res/shaders/kernel.vert",
                                 "res/shaders/debug.frag","");
    cull = new GLUtils::Program("res/shaders/cull.vert",
                                "res/shaders/cull.geom",
                                "res/shaders/simple.frag","outVisible");
    
    CHECK_GL_ERRORS();
}


void AppManager::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
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

