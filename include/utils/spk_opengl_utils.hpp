#pragma once

#include <GL/glew.h>

#include <GL/gl.h>

#include <array>
#include <iostream>
#include <string>

#include "structure/graphics/texture/spk_texture.hpp"

namespace spk::OpenGLUtils
{
	template <size_t numberOfBytes>
	struct Padding
	{
		std::array<float, numberOfBytes> reserved;
	};

	std::wstring to_wstring(const GLenum &p_type);

	std::wstring getFramebufferErrorString(GLenum status);

	void convertFormatToGLEnum(spk::Texture::Format p_format, GLint &p_internalFormat, GLenum &p_externalFormat);

	void GLAPIENTRY openGLDebugMessageCallback(
		GLenum p_source, GLenum p_type, GLuint p_id, GLenum p_severity, GLsizei p_length, const GLchar *p_message, const void *p_userParam);

	void LogGLErrors(const char* p_where);
	void PrintShaderLog(GLuint p_shader, const char* p_label);
	void PrintProgramLog(GLuint p_prog, const char* p_label);
	void DumpActiveAttribs(GLuint p_prog);
	void DumpActiveUniformsAndSamplers(GLuint p_prog);
	void DumpUniformBlocks(GLuint p_prog);
	void DumpPreDrawState();
	void DumpUBOBindingsForUsedBlocks(GLuint p_prog);
	void DumpTexture2DCompletenessForUnit(GLint p_unit);
	void CheckIndexBufferCapacityVsCount(GLsizei p_count, GLenum p_indexType);
}

#define SPK_PP_CAT_IMPL(a, b) a##b
#define SPK_PP_CAT(a, b) SPK_PP_CAT_IMPL(a, b)

#define SPK_PADDING(value) spk::OpenGLUtils::Padding<value> SPK_PP_CAT(spk_padding_, __COUNTER__)
