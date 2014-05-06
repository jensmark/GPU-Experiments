
#include "Visualizer.h"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>

// Static decleration
VirtualTrackball Visualizer::trackball;

Visualizer::Visualizer(unsigned int width,unsigned int height){
    this->width = width;
    this->height = height;

}
Visualizer::~Visualizer(){
    delete visualize;
    delete surface_vert;
    delete surface_ind;
    
    glfwDestroyWindow(window);
    glfwTerminate();
}
void Visualizer::init(){
    /* Initialize the library */
    if (glfwInit() != GL_TRUE) {
        THROW_EXCEPTION("Failed to initialize GLFW");
    }
    glfwSetErrorCallback(error_callback);
    
    createOpenGLContext();
    
    camera.projection = glm::perspective(45.0f,
            width / (float) height, 1.0f, 50.0f);
	camera.view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.0f));
    model = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f,-0.5f,-0.5f));
    
    trackball.setWindowSize(width, height);
    
    setOpenGLStates();
    createProgram();
    createVAO();
}
void Visualizer::render(GLuint tex){
    int width, height;
    glfwGetFramebufferSize(window,&width,&height);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    visualize->use();
    
    glm::mat4 view_matrix_new = camera.view*trackball.getTransform();
    
    glUniform1i(visualize->getUniform("tex"),0);
    
    glUniformMatrix4fv(visualize->getUniform("projection_matrix"), 1, 0, glm::value_ptr(camera.projection));
    glUniformMatrix4fv(visualize->getUniform("model_matrix"), 1, 0, glm::value_ptr(model));
    glUniformMatrix4fv(visualize->getUniform("view_matrix"), 1, 0, glm::value_ptr(view_matrix_new));
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    
    glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(restart_token);
    
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLE_STRIP, indices_count, GL_UNSIGNED_INT, NULL);
    glBindVertexArray(0);
    
    visualize->disuse();
    
    glDisable(GL_PRIMITIVE_RESTART);
    
    glFinish();
    CHECK_GL_ERRORS();

}
void Visualizer::createOpenGLContext(){
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
    window = glfwCreateWindow(width, height, "GL App", NULL, NULL);
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
void Visualizer::setOpenGLStates(){
    glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
    
    glViewport(0, 0, width, height);
	glClearColor(1.0, 1.0, 1.0, 1.0);
    
    CHECK_GL_ERRORS();
}
void Visualizer::createProgram(){
    visualize   = new GLUtils::Program("res/shaders/heightmap.vert","res/shaders/surface.frag");
    CHECK_GL_ERRORS();
}

void Visualizer::createVAO(){
    float r = (float)512/(float)512;
    float d = glm::sqrt((float)(256*256+256*256));
    int w = (int)glm::max(d/(glm::sqrt(r*r+1.0))+1.0,10.0);
    int h = (int)glm::max(d/(glm::sqrt((1.0/(r*r))+1.0))+1.0,10.0);
    
    createTriangleStrip(h,w,surface_vert,surface_ind);
    
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    visualize->use();
    surface_vert->bind();
    visualize->setAttributePointer("position", 2);
    surface_ind->bind();
    glBindVertexArray(0);
    
    CHECK_GL_ERRORS();
}

void Visualizer::createTriangleStrip(unsigned int Nx, unsigned int Ny, GLUtils::BO<GL_ARRAY_BUFFER>*& vert, GLUtils::BO<GL_ELEMENT_ARRAY_BUFFER>*& ind){
    float dx = 1.0f/static_cast<float>(Nx);
	float dy = 1.0f/static_cast<float>(Ny);
    
    std::vector<GLfloat>    vertices;
    std::vector<GLuint>     indices;
    
	//Vertices
	vertices.reserve((Nx+1)*(Ny+1));
	for (unsigned int j=0; j<=Ny; ++j) {
		for (unsigned int i=0; i<=Nx; ++i) {
			vertices.push_back(i*dx);	//x
			vertices.push_back(j*dy);	//y
		}
	}
    vert = new GLUtils::BO<GL_ARRAY_BUFFER>(vertices.data(),sizeof(GLfloat)*(GLuint)vertices.size());
    
	restart_token = Nx * Ny * 2;
    
	//Indices
	for (unsigned int j=0; j<Ny; ++j) {
		for (unsigned int i=0; i<=Nx; ++i) {
			indices.push_back(    j*(Nx+1)+i);
			indices.push_back((j+1)*(Nx+1)+i);
		}
		indices.push_back(restart_token);
	}
    indices_count = (GLuint)indices.size();
    ind = new GLUtils::BO<GL_ELEMENT_ARRAY_BUFFER>(indices.data(),sizeof(GLfloat)*(GLuint)indices.size());
}

void Visualizer::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

void Visualizer::mouse_callback(GLFWwindow* window, int button, int action, int mods){
    double x,y;
    glfwGetCursorPos(window, &x, &y);
    
    if (action == GLFW_PRESS) {
        trackball.rotateBegin((int)x, (int)y);
    } else {
        trackball.rotateEnd((int)x, (int)y);
    }
}

void Visualizer::cursor_callback(GLFWwindow* window, double x, double y){
    trackball.rotate(x,y);
}
