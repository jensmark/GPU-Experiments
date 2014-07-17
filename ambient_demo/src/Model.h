#ifndef _MODEL_H__
#define _MODEL_H__

#include <memory>
#include <string>
#include <vector>
#include <map>

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "GLUtils.hpp"

/**
 * Materiale defining a mesh materiale
 */
struct Materiale {
    Materiale(){
        diffuse     = glm::vec3(1.0f,1.0f,1.0f);
        specular    = glm::vec3(1.0f,1.0f,1.0f);
        ambient     = glm::vec3(1.0f,1.0f,1.0f);
        shininess   = 0.0f;
        textureID   = 0;
    }
    
	glm::vec3 diffuse;
	glm::vec3 specular;
	glm::vec3 ambient;
	float shininess;
	int textureID;
};

/**
 * Vertex
 */
struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
};

/**
 * Image loaded with devil
 */
struct Image{
	std::string path;
	int width;
	int height;
	int components;
	std::vector<unsigned char> data;
};


/**
 * One mesh in the scene
 */
struct MeshPart {
	MeshPart() {
		transform = glm::mat4(1.0f);
		first = 0;
		count = 0;
	}
    
	glm::mat4 transform;
	unsigned int first;
	unsigned int count;
	std::vector<MeshPart> children;
	Materiale materiale;
};

class Model {
public:
	Model(std::string filename, bool invert=0);
	~Model();
    
	/**
	 * Returns the root mesh to start recursive rendering
	 */
	inline MeshPart& getMesh() {return root;}
    
	/**
	 * Returns vertex VBO for entire scene
	 */
	inline std::shared_ptr<GLUtils::BO<GL_ARRAY_BUFFER>> getVertices() {return vertices;}
    
	/**
	 * Returns index VBO for entire scene
	 */
	inline std::shared_ptr<GLUtils::BO<GL_ELEMENT_ARRAY_BUFFER>> getIndices() {return indices;}
    
private:
	/**
	 * Recursivly loads scene using assimp
	 */
	void loadRecursive(MeshPart& part, bool invert,
                       std::vector<unsigned int>& indices, std::vector<Vertex>& vertex_data, const aiScene* scene, const aiNode* node);
    
	/**
	 * Applies a materiale to a mesh struct
	 */
	void applyMateriale(const aiMaterial* materiale, Materiale& meshMat);
    
	/**
	 * Loades a texture into opengl and returns the texture ID
	 */
	int loadTexture(Image& image);
    
	/**
	 * Loades an image using devil image loader
	 */
	void loadImage(Image& image, const char* filepath);
    
    
	const aiScene* scene;
	MeshPart root;
    
	std::string folderPath;
    
	std::shared_ptr<GLUtils::BO<GL_ELEMENT_ARRAY_BUFFER>> indices;
	std::shared_ptr<GLUtils::BO<GL_ARRAY_BUFFER>> vertices;
    
	glm::vec3 min_dim;
	glm::vec3 max_dim;
    
	unsigned int n_vertices;
    
	std::map<std::string, unsigned int> textureID_map;
};

#endif