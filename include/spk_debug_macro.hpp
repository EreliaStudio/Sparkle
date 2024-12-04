#pragma once

#include "structure/spk_iostream.hpp"
#include "utils/spk_opengl_utils.hpp"
#include "utils/spk_string_utils.hpp"

#define DEBUG_INFO() spk::StringUtils::stringToWString(__FILE__) + L":" + std::to_wstring(__LINE__)
#define DEBUG_LINE() spk::cout() <<  DEBUG_INFO() << std::endl

#define GL_DEBUG_LINE()                                                                                                                              \
	{                                                                                                                                                \
		GLenum err = glGetError();                                                                                                                   \
		if (err != GL_NO_ERROR)                                                                                                                      \
		{                                                                                                                                            \
			spk::cout() <<  __FUNCTION__ << ":" << __LINE__ << " - OpenGL error: " << err << std::endl;                                                 \
		}                                                                                                                                            \
		else                                                                                                                                         \
		{                                                                                                                                            \
			DEBUG_LINE();                                                                                                                            \
		}                                                                                                                                            \
	}
#define FRAMEBUFFER_DEBUG_LINE()                                                                                                                     \
	{                                                                                                                                                \
		GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);                                                                                  \
		if (fbStatus != GL_FRAMEBUFFER_COMPLETE)                                                                                                     \
		{                                                                                                                                            \
			spk::cout() <<  __FILE__ << L":" << __LINE__ << L" - Framebuffer error: " << spk::OpenGLUtils::getFramebufferErrorString(fbStatus)          \
					  << std::endl;                                                                                                                  \
		}                                                                                                                                            \
		else                                                                                                                                         \
		{                                                                                                                                            \
			DEBUG_LINE();                                                                                                                            \
		}                                                                                                                                            \
	}
