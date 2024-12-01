#pragma once

#include <string>  // For std::wstring
#include <iostream>
#include "GL/glew.h"
#include "GL/gl.h"

namespace spk::OpenGLUtils
{
	std::wstring to_wstring(const GLenum& p_type);
}