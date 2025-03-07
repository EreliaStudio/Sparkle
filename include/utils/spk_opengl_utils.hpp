#pragma once

#include <string>  // For std::wstring
#include <iostream>
#include "GL/glew.h"
#include "GL/gl.h"

#include "structure/graphics/texture/spk_texture.hpp"

namespace spk::OpenGLUtils
{
	std::wstring to_wstring(const GLenum& p_type);

	std::wstring getFramebufferErrorString(GLenum status);

	void _retrieveOpenGLFormat(spk::Texture::Format p_format, GLint& p_internalFormat, GLenum& p_externalFormat);
}