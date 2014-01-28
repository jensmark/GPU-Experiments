#ifndef _TEXTUREFBO_HPP__
#define _TEXTUREFBO_HPP__

#include "GLUtils/GLUtils.hpp"

class TextureFBO {
public:
	TextureFBO(unsigned int width, unsigned int height, unsigned int targets = 1, int format = GL_RGBA32F);
	~TextureFBO();

	void bind();
	static void unbind();

	unsigned int getWidth() {return width; }
	unsigned int getHeight() {return height; }

	GLuint getTexture(unsigned int target) { return texture[target]; }

private:
	GLuint fbo;
	GLuint depth;
	GLuint* texture;
	unsigned int width, height;
};

#endif