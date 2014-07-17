#include "Model.h"

#include <IL/IL.h>
#include <IL/ILU.h>

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

Model::Model(std::string filename, bool invert) {
	//std::vector<float> vertex_data, normal_data;
	std::vector<Vertex> vertex_data;
	std::vector<unsigned int> indices_data;
    
	int pos = filename.find_last_of("/");
	if(pos == std::string::npos)
		pos = filename.find_last_of("\\");
    
	folderPath = filename.substr(0, pos + 1);
    
	scene = aiImportFile(filename.c_str(), aiProcessPreset_TargetRealtime_Quality);// | aiProcess_FlipWindingOrder);
	if (!scene) {
		std::string log = aiGetErrorString();
		THROW_EXCEPTION(log);
	}
	
	max_dim = -glm::vec3(std::numeric_limits<float>().max());
	min_dim = glm::vec3(std::numeric_limits<float>().max());
    
	//Load the model recursively into data
	loadRecursive(root, invert, indices_data, vertex_data, scene, scene->mRootNode);
    
	float tmp;
    tmp = max_dim.x - min_dim.x;
    tmp = max_dim.y - min_dim.y > tmp ? max_dim.y - min_dim.y : tmp;
    tmp = max_dim.z - min_dim.z > tmp ? max_dim.z - min_dim.z : tmp;
    float scaleFactor = 1.0f / tmp;
	
	glm::vec3 center = (max_dim + min_dim);
	center /= 2;
    
	root.transform = glm::scale(glm::mat4(1), glm::vec3(scaleFactor));
	root.transform = glm::translate(root.transform, -center);
    
	n_vertices = indices_data.size();
    
    std::cout << "Loaded model with " << n_vertices << " vertices" << std::endl;
    
	//Create the VBOs from the data.
	if (fmod(static_cast<float>(n_vertices), 3.0f) < 0.000001f)
	{
		indices.reset(new GLUtils::BO<GL_ELEMENT_ARRAY_BUFFER>(indices_data.data(), indices_data.size() * sizeof(unsigned int)));
		vertices.reset(new GLUtils::BO<GL_ARRAY_BUFFER>(vertex_data.data(), vertex_data.size() * sizeof(Vertex)));
        
        std::cout << "Created and uploaded VBOs" << std::endl;
	}
	else
	{
		THROW_EXCEPTION("The number of vertices in the mesh is wrong");
	}
}

Model::~Model() {
    
}

void Model::loadRecursive(MeshPart& part, bool invert,
                          std::vector<unsigned int>& indices, std::vector<Vertex>& vertex_data, const aiScene* scene, const aiNode* node) {
    std::cout << "loading one part" << std::endl;
    
	//update transform matrix. notice that we also transpose it
	aiMatrix4x4 m = node->mTransformation;
	for (int j=0; j<4; ++j)
		for (int i=0; i<4; ++i)
			part.transform[j][i] = m[i][j];
    
	// draw all meshes assigned to this node
	for (unsigned int n=0; n < node->mNumMeshes; ++n) {
        std::cout << "\tloading mesh part " << n << std::endl;
		const struct aiMesh* mesh = scene->mMeshes[node->mMeshes[n]];
        
		//apply_material(scene->mMaterials[mesh->mMaterialIndex]);
		applyMateriale(scene->mMaterials[mesh->mMaterialIndex], part.materiale);
        
		part.first = indices.size();
		part.count = mesh->mNumFaces * 3;
        
		//Allocate data
		indices.reserve(part.first + part.count);
		vertex_data.reserve(vertex_data.size() + mesh->mNumVertices);
        
		for (unsigned int t = 0; t < mesh->mNumFaces; ++t) {
			const struct aiFace* face = &mesh->mFaces[t];
            
			if(face->mNumIndices != 3)
				THROW_EXCEPTION("Only triangle meshes are supported");
            
			for(unsigned int i = 0; i < face->mNumIndices; i++) {
				int index = face->mIndices[i];
				indices.push_back(vertex_data.size()  + index);
			}
		}
        
		for(unsigned int t = 0; t < mesh->mNumVertices; ++t){
			Vertex v;
			unsigned int index = t;
			// set vertex position
			v.position = glm::vec3(mesh->mVertices[index].x, mesh->mVertices[index].y, mesh->mVertices[index].z);
            
			// compute bounding box
			max_dim.x = v.position.x < max_dim.x ? max_dim.x : v.position.x;
			max_dim.y = v.position.y < max_dim.y ? max_dim.y : v.position.y;
			max_dim.z = v.position.z < max_dim.z ? max_dim.z : v.position.z;
            
			min_dim.x = v.position.x > min_dim.x ? min_dim.x : v.position.x;
			min_dim.y = v.position.y > min_dim.y ? min_dim.y : v.position.y;
			min_dim.z = v.position.z > min_dim.z ? min_dim.z : v.position.z;
            
			// get normal
			if(mesh->HasNormals())
				v.normal = glm::vec3(mesh->mNormals[index].x, mesh->mNormals[index].y, mesh->mNormals[index].z);
            
			// get uv coord
			if(mesh->HasTextureCoords(0))
				v.texCoord = glm::vec2(mesh->mTextureCoords[0][index].x, mesh->mTextureCoords[0][index].y);
            
			vertex_data.push_back(v);
		}
	}
    
	// load all children
	for (unsigned int n = 0; n < node->mNumChildren; ++n) {
		part.children.push_back(MeshPart());
		loadRecursive(part.children.back(), invert, indices, vertex_data, scene, node->mChildren[n]);
	}
}

void Model::applyMateriale(const aiMaterial* materiale, Materiale& meshMat)
{
    std::cout << "Applying mesh material" << std::endl;
	aiString texPath;
    if(AI_SUCCESS == materiale->GetTexture(aiTextureType_DIFFUSE, 0, &texPath)){
		std::string fullPath = folderPath + texPath.C_Str();
        
		// Check if this texture was already loaded.
		std::map<std::string, unsigned int>::iterator it = textureID_map.find(fullPath);
		if(it == textureID_map.end()){
			Image img;
			loadImage(img, fullPath.c_str());
			meshMat.textureID = loadTexture(img);
		}else{
			meshMat.textureID = it->second;
		}
        
        std::cout << "loaded texture: " << meshMat.textureID << std::endl;
    }else{
		Image img;
        
		// generate white dummy 16x16 texture so we don't have to
		// switch shaders for untextured meshes
		img.components = 3;
		img.height = 1;
		img.width = 1;
		img.data.resize(1 * 1 * 3);
		for(int i = 0; i < 1 * 1 * 3; i++){
			img.data[i] = (unsigned char) 255;
		}
        
		meshMat.textureID = loadTexture(img);
        std::cout << "Created a dummy texture: " << meshMat.textureID << std::endl;
	}
    
	// Fetch material specs
	aiColor4D diffuse;
	if(AI_SUCCESS == aiGetMaterialColor(materiale, AI_MATKEY_COLOR_DIFFUSE, &diffuse)){
		meshMat.diffuse = glm::vec3(diffuse.r, diffuse.g, diffuse.b);
	}
    
	aiColor4D ambient;
	if(AI_SUCCESS == aiGetMaterialColor(materiale, AI_MATKEY_COLOR_AMBIENT, &ambient)){
		meshMat.ambient = glm::vec3(ambient.r, ambient.g, ambient.b);
	}
    
	aiColor4D specular;
	if(AI_SUCCESS == aiGetMaterialColor(materiale, AI_MATKEY_COLOR_SPECULAR, &specular)){
		meshMat.specular = glm::vec3(specular.r, specular.g, specular.b);
	}
    
	float shininess = 0.0f;
	unsigned int max;
	aiGetMaterialFloatArray(materiale, AI_MATKEY_SHININESS, &shininess, &max);
	meshMat.shininess = shininess;
}

void Model::loadImage(Image& image, const char* filepath)
{
    std::cout << "Trying to load image at filepath: " << filepath << std::endl;
    
	ILuint imageName; // The image name to return.
    ilGenImages(1, &imageName); // Grab a new image name.
    
	ilBindImage(imageName);
    
	if (!ilLoadImage(filepath)) {
		ILenum Error;
		while ((Error = ilGetError()) != IL_NO_ERROR) {
			std::cout << Error << " " << iluErrorString(Error) << std::endl;
		}
		ilDeleteImages(1, &imageName); // Delete the image name.
	}
    
	image.path = filepath;
	image.width = ilGetInteger(IL_IMAGE_WIDTH); // getting image width
	image.height = ilGetInteger(IL_IMAGE_HEIGHT); // and height
	image.components = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);
	if(image.components < 3) image.components = 3;
	int memory_needed = image.width * image.height * image.components;
	image.data.resize(memory_needed); //Allocate memory
    
	// finally get the image data, and delete the il-image.
	unsigned int type = IL_RGB;
	if(image.components == 4)
		type = IL_RGBA;
	ilCopyPixels(0, 0, 0, image.width, image.height, 1, type, IL_UNSIGNED_BYTE, &image.data[0]);
	ilDeleteImages(1, &imageName);
}

int Model::loadTexture(Image& image)
{
    
	unsigned int id;
    
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
    
	GLenum format = GL_RGB;
	if(image.components == 4)
		format = GL_RGBA;
    
	glTexImage2D(GL_TEXTURE_2D, 0, format, image.width, image.height,
                 0, format, GL_UNSIGNED_BYTE, &image.data[0]);
    
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
	glBindTexture(GL_TEXTURE_2D, 0);
    
	// Insert image into map for easier access later
	textureID_map.insert(std::pair<std::string, unsigned int>(image.path, id));
	
	return id;
}