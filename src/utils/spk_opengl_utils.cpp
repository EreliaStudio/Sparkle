#include "utils/spk_opengl_utils.hpp"

namespace spk::OpenGLUtils
{
	std::wstring to_wstring(const GLenum &p_type)
	{
		switch (p_type)
		{
		case GL_NO_ERROR:
			return (L"No error");
		case GL_INVALID_ENUM:
			return (L"Invalid enum");
		case GL_INVALID_VALUE:
			return (L"Invalid value");
		case GL_INVALID_OPERATION:
			return (L"Invalid operation");
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			return (L"Invalid framebuffer operation");
		case GL_OUT_OF_MEMORY:
			return (L"Out of memory");
		case GL_STACK_UNDERFLOW:
			return (L"Stack underflow");
		case GL_STACK_OVERFLOW:
			return (L"Stack overflow");

		case GL_BYTE:
			return (L"GL_BYTE");
		case GL_UNSIGNED_BYTE:
			return (L"GL_UNSIGNED_BYTE");
		case GL_UNSIGNED_SHORT:
			return (L"GL_UNSIGNED_SHORT");
		case GL_2_BYTES:
			return (L"GL_2_BYTES");
		case GL_3_BYTES:
			return (L"GL_3_BYTES");
		case GL_4_BYTES:
			return (L"GL_4_BYTES");

		case GL_FLOAT:
			return (L"float");
		case GL_FLOAT_VEC2:
			return (L"vec2");
		case GL_FLOAT_VEC3:
			return (L"vec3");
		case GL_FLOAT_VEC4:
			return (L"vec4");
		case GL_DOUBLE:
			return (L"double");
		case GL_DOUBLE_VEC2:
			return (L"dvec2");
		case GL_DOUBLE_VEC3:
			return (L"dvec3");
		case GL_DOUBLE_VEC4:
			return (L"dvec4");
		case GL_INT:
			return (L"int");
		case GL_INT_VEC2:
			return (L"ivec2");
		case GL_INT_VEC3:
			return (L"ivec3");
		case GL_INT_VEC4:
			return (L"ivec4");
		case GL_UNSIGNED_INT:
			return (L"unsigned int");
		case GL_UNSIGNED_INT_VEC2:
			return (L"uvec2");
		case GL_UNSIGNED_INT_VEC3:
			return (L"uvec3");
		case GL_UNSIGNED_INT_VEC4:
			return (L"uvec4");
		case GL_BOOL:
			return (L"bool");
		case GL_BOOL_VEC2:
			return (L"bvec2");
		case GL_BOOL_VEC3:
			return (L"bvec3");
		case GL_BOOL_VEC4:
			return (L"bvec4");
		case GL_FLOAT_MAT2:
			return (L"mat2");
		case GL_FLOAT_MAT3:
			return (L"mat3");
		case GL_FLOAT_MAT4:
			return (L"mat4");
		case GL_FLOAT_MAT2x3:
			return (L"mat2x3");
		case GL_FLOAT_MAT2x4:
			return (L"mat2x4");
		case GL_FLOAT_MAT3x2:
			return (L"mat3x2");
		case GL_FLOAT_MAT3x4:
			return (L"mat3x4");
		case GL_FLOAT_MAT4x2:
			return (L"mat4x2");
		case GL_FLOAT_MAT4x3:
			return (L"mat4x3");
		case GL_DOUBLE_MAT2:
			return (L"dmat2");
		case GL_DOUBLE_MAT3:
			return (L"dmat3");
		case GL_DOUBLE_MAT4:
			return (L"dmat4");
		case GL_DOUBLE_MAT2x3:
			return (L"dmat2x3");
		case GL_DOUBLE_MAT2x4:
			return (L"dmat2x4");
		case GL_DOUBLE_MAT3x2:
			return (L"dmat3x2");
		case GL_DOUBLE_MAT3x4:
			return (L"dmat3x4");
		case GL_DOUBLE_MAT4x2:
			return (L"dmat4x2");
		case GL_DOUBLE_MAT4x3:
			return (L"dmat4x3");
		case GL_SAMPLER_1D:
			return (L"sampler1D");
		case GL_SAMPLER_2d:
			return (L"sampler2D");
		case GL_SAMPLER_3D:
			return (L"sampler3D");
		case GL_SAMPLER_CUBE:
			return (L"samplerCube");
		case GL_SAMPLER_1D_SHADOW:
			return (L"sampler1DShadow");
		case GL_SAMPLER_2d_SHADOW:
			return (L"sampler2DShadow");
		case GL_SAMPLER_1D_ARRAY:
			return (L"sampler1DArray");
		case GL_SAMPLER_2d_ARRAY:
			return (L"sampler2DArray");
		case GL_SAMPLER_1D_ARRAY_SHADOW:
			return (L"sampler1DArrayShadow");
		case GL_SAMPLER_2d_ARRAY_SHADOW:
			return (L"sampler2DArrayShadow");
		case GL_SAMPLER_2d_MULTISAMPLE:
			return (L"sampler2DMS");
		case GL_SAMPLER_2d_MULTISAMPLE_ARRAY:
			return (L"sampler2DMSArray");
		case GL_SAMPLER_CUBE_SHADOW:
			return (L"samplerCubeShadow");
		case GL_SAMPLER_BUFFER:
			return (L"samplerBuffer");
		case GL_SAMPLER_2d_RECT:
			return (L"sampler2DRect");
		case GL_SAMPLER_2d_RECT_SHADOW:
			return (L"sampler2DRectShadow");
		case GL_INT_SAMPLER_1D:
			return (L"isampler1D");
		case GL_INT_SAMPLER_2d:
			return (L"isampler2D");
		case GL_INT_SAMPLER_3D:
			return (L"isampler3D");
		case GL_INT_SAMPLER_CUBE:
			return (L"isamplerCube");
		case GL_INT_SAMPLER_1D_ARRAY:
			return (L"isampler1DArray");
		case GL_INT_SAMPLER_2d_ARRAY:
			return (L"isampler2DArray");
		case GL_INT_SAMPLER_2d_MULTISAMPLE:
			return (L"isampler2DMS");
		case GL_INT_SAMPLER_2d_MULTISAMPLE_ARRAY:
			return (L"isampler2DMSArray");
		case GL_INT_SAMPLER_BUFFER:
			return (L"isamplerBuffer");
		case GL_INT_SAMPLER_2d_RECT:
			return (L"isampler2DRect");
		case GL_UNSIGNED_INT_SAMPLER_1D:
			return (L"usampler1D");
		case GL_UNSIGNED_INT_SAMPLER_2d:
			return (L"usampler2D");
		case GL_UNSIGNED_INT_SAMPLER_3D:
			return (L"usampler3D");
		case GL_UNSIGNED_INT_SAMPLER_CUBE:
			return (L"usamplerCube");
		case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
			return (L"usampler2DArray");
		case GL_UNSIGNED_INT_SAMPLER_2d_ARRAY:
			return (L"usampler2DArray");
		case GL_UNSIGNED_INT_SAMPLER_2d_MULTISAMPLE:
			return (L"usampler2DMS");
		case GL_UNSIGNED_INT_SAMPLER_2d_MULTISAMPLE_ARRAY:
			return (L"usampler2DMSArray");
		case GL_UNSIGNED_INT_SAMPLER_BUFFER:
			return (L"usamplerBuffer");
		case GL_UNSIGNED_INT_SAMPLER_2d_RECT:
			return (L"usampler2DRect");
		case GL_IMAGE_1D:
			return (L"image1D");
		case GL_IMAGE_2d:
			return (L"image2D");
		case GL_IMAGE_3D:
			return (L"image3D");
		case GL_IMAGE_2d_RECT:
			return (L"image2DRect");
		case GL_IMAGE_CUBE:
			return (L"imageCube");
		case GL_IMAGE_BUFFER:
			return (L"imageBuffer");
		case GL_IMAGE_1D_ARRAY:
			return (L"image1DArray");
		case GL_IMAGE_2d_ARRAY:
			return (L"image2DArray");
		case GL_IMAGE_2d_MULTISAMPLE:
			return (L"image2DMS");
		case GL_IMAGE_2d_MULTISAMPLE_ARRAY:
			return (L"image2DMSArray");
		case GL_INT_IMAGE_1D:
			return (L"iimage1D");
		case GL_INT_IMAGE_2d:
			return (L"iimage2D");
		case GL_INT_IMAGE_3D:
			return (L"iimage3D");
		case GL_INT_IMAGE_2d_RECT:
			return (L"iimage2DRect");
		case GL_INT_IMAGE_CUBE:
			return (L"iimageCube");
		case GL_INT_IMAGE_BUFFER:
			return (L"iimageBuffer");
		case GL_INT_IMAGE_1D_ARRAY:
			return (L"iimage1DArray");
		case GL_INT_IMAGE_2d_ARRAY:
			return (L"iimage2DArray");
		case GL_INT_IMAGE_2d_MULTISAMPLE:
			return (L"iimage2DMS");
		case GL_INT_IMAGE_2d_MULTISAMPLE_ARRAY:
			return (L"iimage2DMSArray");
		case GL_UNSIGNED_INT_IMAGE_1D:
			return (L"uimage1D");
		case GL_UNSIGNED_INT_IMAGE_2d:
			return (L"uimage2D");
		case GL_UNSIGNED_INT_IMAGE_3D:
			return (L"uimage3D");
		case GL_UNSIGNED_INT_IMAGE_2d_RECT:
			return (L"uimage2DRect");
		case GL_UNSIGNED_INT_IMAGE_CUBE:
			return (L"uimageCube");
		case GL_UNSIGNED_INT_IMAGE_BUFFER:
			return (L"uimageBuffer");
		case GL_UNSIGNED_INT_IMAGE_1D_ARRAY:
			return (L"uimage1DArray");
		case GL_UNSIGNED_INT_IMAGE_2d_ARRAY:
			return (L"uimage2DArray");
		case GL_UNSIGNED_INT_IMAGE_2d_MULTISAMPLE:
			return (L"uimage2DMS");
		case GL_UNSIGNED_INT_IMAGE_2d_MULTISAMPLE_ARRAY:
			return (L"uimage2DMSArray");
		case GL_UNSIGNED_INT_ATOMIC_COUNTER:
			return (L"atomic_uint");

		case GL_ARRAY_BUFFER:
			return (L"Vertex attributes");
		case GL_ATOMIC_COUNTER_BUFFER:
			return (L"Atomic counter storage");
		case GL_COPY_READ_BUFFER:
			return (L"Buffer copy source");
		case GL_COPY_WRITE_BUFFER:
			return (L"Buffer copy destination");
		case GL_DISPATCH_INDIRECT_BUFFER:
			return (L"Indirect compute dispatch commands");
		case GL_DRAW_INDIRECT_BUFFER:
			return (L"Indirect command arguments");
		case GL_ELEMENT_ARRAY_BUFFER:
			return (L"Vertex array indices");
		case GL_PIXEL_PACK_BUFFER:
			return (L"Pixel read target");
		case GL_PIXEL_UNPACK_BUFFER:
			return (L"Texture data source");
		case GL_QUERY_BUFFER:
			return (L"Query result buffer");
		case GL_SHADER_STORAGE_BUFFER:
			return (L"Read-write storage for shaders");
		case GL_TEXTURE_BUFFER:
			return (L"Texture data buffer");
		case GL_TRANSFORM_FEEDBACK_BUFFER:
			return (L"Transform feedback buffer");
		case GL_UNIFORM_BUFFER:
			return (L"Uniform block storage");

		case GL_TEXTURE_2D:
			return (L"GL_TEXTURE_2D");
		case GL_RGB:
			return (L"GL_RGB");
		case GL_RGBA:
			return (L"GL_RGBA");
		case GL_BGR:
			return (L"GL_BGR");
		case GL_BGRA:
			return (L"GL_BGRA");
		case GL_RED:
			return (L"GL_RED");

		default:
			return (L"not a GLenum type");
		}
	}

	std::wstring getFramebufferErrorString(GLenum p_status)
	{
		switch (p_status)
		{
		case GL_FRAMEBUFFER_COMPLETE:
			return (L"Framebuffer complete");
		case GL_FRAMEBUFFER_UNDEFINED:
			return (L"Framebuffer undefined (GL_FRAMEBUFFER_UNDEFINED)");
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			return (L"Incomplete attachment (GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)");
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			return (L"Missing attachment (GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)");
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			return (L"Incomplete draw buffer (GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER)");
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			return (L"Incomplete read buffer (GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER)");
		case GL_FRAMEBUFFER_UNSUPPORTED:
			return (L"Framebuffer unsupported (GL_FRAMEBUFFER_UNSUPPORTED)");
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			return (L"Incomplete multisample (GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)");
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
			return (L"Incomplete layer targets (GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS)");
		default:
			return (L"Unknown framebuffer error");
		}
	}

	void convertFormatToGLEnum(spk::Texture::Format p_format, GLint &p_internalFormat, GLenum &p_externalFormat)
	{
		switch (p_format)
		{
		case spk::Texture::Format::RGB:
			p_internalFormat = GL_RGB8;
			p_externalFormat = GL_RGB;
			break;

		case spk::Texture::Format::RGBA:
			p_internalFormat = GL_RGBA8;
			p_externalFormat = GL_RGBA;
			break;

		case spk::Texture::Format::GreyLevel:
			p_internalFormat = GL_R8;
			p_externalFormat = GL_RED;
			break;

		case spk::Texture::Format::DualChannel:
			p_internalFormat = GL_RG8;
			p_externalFormat = GL_RG;
			break;

		default:
			p_internalFormat = 0;
			p_externalFormat = 0;
			break;
		}
	}

	void GLAPIENTRY openGLDebugMessageCallback(
		GLenum p_source, GLenum p_type, GLuint p_id, GLenum p_severity, GLsizei p_length, const GLchar *p_message, const void *p_userParam)
	{
		if (p_id == 131169 || p_id == 131185 || p_id == 131218 || p_id == 131204)
		{
			return;
		}

		spk::cout << "---------------" << std::endl;

		switch (p_source)
		{
		case GL_DEBUG_SOURCE_API:
			spk::cout << "Source: API" << std::endl;
			break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
			spk::cout << "Source: Window System" << std::endl;
			break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER:
			spk::cout << "Source: Shader Compiler" << std::endl;
			break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:
			spk::cout << "Source: Third Party" << std::endl;
			break;
		case GL_DEBUG_SOURCE_APPLICATION:
			spk::cout << "Source: Application" << std::endl;
			break;
		case GL_DEBUG_SOURCE_OTHER:
			spk::cout << "Source: Other" << std::endl;
			break;
		}

		switch (p_type)
		{
		case GL_DEBUG_TYPE_ERROR:
			spk::cout << "Type: Error" << std::endl;
			break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			spk::cout << "Type: Deprecated Behaviour" << std::endl;
			break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			spk::cout << "Type: Undefined Behaviour" << std::endl;
			break;
		case GL_DEBUG_TYPE_PORTABILITY:
			spk::cout << "Type: Portability" << std::endl;
			break;
		case GL_DEBUG_TYPE_PERFORMANCE:
			spk::cout << "Type: Performance" << std::endl;
			break;
		case GL_DEBUG_TYPE_MARKER:
			spk::cout << "Type: Marker" << std::endl;
			break;
		case GL_DEBUG_TYPE_PUSH_GROUP:
			spk::cout << "Type: Push Group" << std::endl;
			break;
		case GL_DEBUG_TYPE_POP_GROUP:
			spk::cout << "Type: Pop Group" << std::endl;
			break;
		case GL_DEBUG_TYPE_OTHER:
			spk::cout << "Type: Other" << std::endl;
			break;
		}

		switch (p_severity)
		{
		case GL_DEBUG_SEVERITY_HIGH:
			spk::cout << "Severity: high" << std::endl;
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
			spk::cout << "Severity: medium" << std::endl;
			break;
		case GL_DEBUG_SEVERITY_LOW:
			spk::cout << "Severity: low" << std::endl;
			break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:
			spk::cout << "Severity: notification" << std::endl;
			break;
		}

		spk::cout << "Debug message (" << p_id << "): " << p_message << std::endl;
		if (p_severity != GL_DEBUG_SEVERITY_NOTIFICATION)
		{
			GENERATE_ERROR("Unexpected opengl error detected");
		}
	}

	void LogGLErrors(const char *p_where)
	{
		GLenum err;
		bool any = false;
		while ((err = glGetError()) != GL_NO_ERROR)
		{
			any = true;
			std::cerr << "[GL] Error at " << p_where << ": 0x" << std::hex << err << std::dec << "\n";
		}
		if (any)
		{
			std::cerr << std::flush;
		}
	}

	void PrintShaderLog(GLuint p_shader, const char *p_label)
	{
		GLint status = 0, len = 0;
		glGetShaderiv(p_shader, GL_COMPILE_STATUS, &status);
		glGetShaderiv(p_shader, GL_INFO_LOG_LENGTH, &len);
		std::vector<char> log((size_t)std::max(1, len));
		if (len > 1)
		{
			glGetShaderInfoLog(p_shader, len, nullptr, log.data());
		}
		std::cerr << "[Shader] " << p_label << " compile status=" << ((status != 0) ? "OK" : "FAIL") << " log:\n" << log.data() << "\n";
	}

	void PrintProgramLog(GLuint p_prog, const char *p_label)
	{
		GLint link = 0, len = 0;
		glGetProgramiv(p_prog, GL_LINK_STATUS, &link);
		glGetProgramiv(p_prog, GL_INFO_LOG_LENGTH, &len);
		std::vector<char> log((size_t)std::max(1, len));
		if (len > 1)
		{
			glGetProgramInfoLog(p_prog, len, nullptr, log.data());
		}
		std::cerr << "[Program] " << p_label << " link status=" << ((link != 0) ? "OK" : "FAIL") << " log:\n" << log.data() << "\n";
	}

	void DumpActiveAttribs(GLuint p_prog)
	{
		GLint nAttribs = 0, maxLen = 0;
		glGetProgramiv(p_prog, GL_ACTIVE_ATTRIBUTES, &nAttribs);
		glGetProgramiv(p_prog, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxLen);
		std::vector<char> name(maxLen);
		std::cerr << "[Program] Active attributes (" << nAttribs << "):\n";
		for (GLint i = 0; i < nAttribs; ++i)
		{
			GLsizei length;
			GLint size;
			GLenum type;
			glGetActiveAttrib(p_prog, (GLuint)i, maxLen, &length, &size, &type, name.data());
			GLint loc = glGetAttribLocation(p_prog, name.data());
			std::cerr << "  - " << name.data() << " @location " << loc << "\n";
		}
	}

	void DumpActiveUniformsAndSamplers(GLuint p_prog)
	{
		GLint nUniforms = 0, maxLen = 0;
		glGetProgramiv(p_prog, GL_ACTIVE_UNIFORMS, &nUniforms);
		glGetProgramiv(p_prog, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLen);
		std::vector<char> name(maxLen);
		std::cerr << "[Program] Active uniforms (" << nUniforms << "):\n";
		for (GLint i = 0; i < nUniforms; ++i)
		{
			GLsizei length;
			GLint size;
			GLenum type;
			glGetActiveUniform(p_prog, (GLuint)i, maxLen, &length, &size, &type, name.data());
			GLint loc = glGetUniformLocation(p_prog, name.data());
			std::cerr << "  - " << name.data() << " (type=0x" << std::hex << type << std::dec << ") loc=" << loc;
			if (type == GL_SAMPLER_2d || type == GL_SAMPLER_2d_ARRAY || type == GL_SAMPLER_CUBE)
			{
				GLint unit = -1;
				if (loc >= 0)
				{
					glGetUniformiv(p_prog, loc, &unit);
				}
				std::cerr << " [sampler -> unit " << unit << "]";
			}
			std::cerr << "\n";
		}
	}

	void DumpUniformBlocks(GLuint p_prog)
	{
		GLint nBlocks = 0, maxLen = 0;
		glGetProgramiv(p_prog, GL_ACTIVE_UNIFORM_BLOCKS, &nBlocks);
		glGetProgramiv(p_prog, GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH, &maxLen);
		std::vector<char> name(maxLen);
		std::cerr << "[Program] Uniform blocks (" << nBlocks << "):\n";
		for (GLint i = 0; i < nBlocks; ++i)
		{
			GLsizei length;
			glGetActiveUniformBlockName(p_prog, (GLuint)i, maxLen, &length, name.data());
			GLint bind = -1;
			glGetActiveUniformBlockiv(p_prog, (GLuint)i, GL_UNIFORM_BLOCK_BINDING, &bind);
			GLint dataSize = 0;
			glGetActiveUniformBlockiv(p_prog, (GLuint)i, GL_UNIFORM_BLOCK_DATA_SIZE, &dataSize);
			std::cerr << "  - " << name.data() << " binding=" << bind << " size=" << dataSize << " bytes\n";
			if (bind < 0)
			{
				std::cerr << "    ! WARNING: block has no binding; with layout(binding=X) this should be X.\n";
			}
		}
	}

	void DumpPreDrawState()
	{
		GLint prog = 0, vao = 0, ebo = 0;
		glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
		glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &ebo);
		std::cerr << "[State] Program=" << prog << " VAO=" << vao << " EBO=" << ebo << "\n";

		// Attribute enables 0..7 (enough for your case)
		for (GLuint i = 0; i < 3; ++i)
		{
			GLboolean enabled = GL_FALSE;
			glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, (GLint *)&enabled);
			GLint size = 0;
			GLenum type = 0;
			GLboolean norm = GL_FALSE;
			GLsizei stride = 0;
			GLvoid *ptr = nullptr;
			glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_SIZE, &size);
			glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_TYPE, (GLint *)&type);
			glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, (GLint *)&norm);
			glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_STRIDE, (GLint *)&stride);
			glGetVertexAttribPointerv(i, GL_VERTEX_ATTRIB_ARRAY_POINTER, &ptr);
			std::cerr << "  [Attrib " << i << "] enabled=" << ((enabled != 0u) ? "Y" : "N") << " size=" << size << " type=0x" << std::hex << type
					  << std::dec << " stride=" << stride << " ptr=" << ptr << "\n";
		}
	}

	void DumpUBOBindingsForUsedBlocks(GLuint p_prog)
	{
		GLint nBlocks = 0;
		glGetProgramiv(p_prog, GL_ACTIVE_UNIFORM_BLOCKS, &nBlocks);
		std::cerr << "[State] UBO bindings for used blocks:\n";
		for (GLint i = 0; i < nBlocks; ++i)
		{
			GLint bind = -1;
			glGetActiveUniformBlockiv(p_prog, (GLuint)i, GL_UNIFORM_BLOCK_BINDING, &bind);
			GLint buffer = 0;
			if (bind >= 0)
			{
				glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, bind, &buffer);
			}
			std::cerr << "  - block#" << i << " binding=" << bind << " -> UBO id=" << buffer;
			if (buffer == 0)
			{
				std::cerr << "  (!! no buffer bound at this binding point)";
			}
			std::cerr << "\n";

			if (buffer != 0)
			{
				GLint sizeBytes = 0;
				glBindBuffer(GL_UNIFORM_BUFFER, (GLuint)buffer);
				glGetBufferParameteriv(GL_UNIFORM_BUFFER, GL_BUFFER_SIZE, &sizeBytes);
				std::cerr << "      UBO size on GPU: " << sizeBytes << " bytes\n";
				glBindBuffer(GL_UNIFORM_BUFFER, 0);
			}
		}
	}

	void DumpTexture2DCompletenessForUnit(GLint p_unit)
	{
		// Save active unit
		GLint curActive = 0;
		glGetIntegerv(GL_ACTIVE_TEXTURE, &curActive);

		glActiveTexture(GL_TEXTURE0 + p_unit);
		GLint tex = 0;
		glGetIntegerv(GL_TEXTURE_BINDING_2d, &tex);
		std::cerr << "[State] Texture unit " << p_unit << " -> 2D tex id=" << tex << "\n";
		if (tex == 0)
		{
			std::cerr << "  (!! no texture bound, sampling will be undefined)\n";
			glActiveTexture(curActive);
			return;
		}

		// Query parameters that affect completeness
		GLint minFilter = 0, magFilter = 0, baseLevel = 0, maxLevel = 0;
		glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &minFilter);
		glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &magFilter);
		glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, &baseLevel);
		glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &maxLevel);

		GLint width0 = 0, height0 = 0, internal0 = 0;
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width0);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height0);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internal0);

		std::cerr << "  minFilter=0x" << std::hex << minFilter << " magFilter=0x" << magFilter << " baseLevel=" << std::dec << baseLevel
				  << " maxLevel=" << maxLevel << "\n";
		std::cerr << "  level0: " << width0 << "x" << height0 << " IF=0x" << std::hex << internal0 << std::dec << "\n";

		// If minFilter is a mipmap mode, ensure level 1 exists (quick heuristic)
		bool needsMips =
			(minFilter == GL_NEAREST_MIPMAP_NEAREST || minFilter == GL_LINEAR_MIPMAP_NEAREST || minFilter == GL_NEAREST_MIPMAP_LINEAR ||
			 minFilter == GL_LINEAR_MIPMAP_LINEAR);

		if (needsMips)
		{
			GLint width1 = 0, height1 = 0, internal1 = 0;
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 1, GL_TEXTURE_WIDTH, &width1);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 1, GL_TEXTURE_HEIGHT, &height1);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 1, GL_TEXTURE_INTERNAL_FORMAT, &internal1);
			std::cerr << "  level1: " << width1 << "x" << height1 << " IF=0x" << std::hex << internal1 << std::dec << "\n";
			if (width1 == 0 || height1 == 0)
			{
				std::cerr << "  (!! mipmap filter selected but no mipmaps defined; this will cause GL_INVALID_OPERATION on draw)\n";
			}
		}

		glActiveTexture(curActive);
	}

	void CheckIndexBufferCapacityVsCount(GLsizei p_count, GLenum p_indexType)
	{
		GLint ebo = 0;
		glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &ebo);
		if (ebo == 0)
		{
			return;
		}
		GLint sizeBytes = 0;
		glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &sizeBytes);
		GLsizei bytesNeeded = p_count * (p_indexType == GL_UNSIGNED_SHORT ? 2 : 4);
		if (bytesNeeded > sizeBytes)
		{
			std::cerr << "!! EBO too small: need " << bytesNeeded << " bytes for " << p_count << " indices, but buffer has " << sizeBytes << "\n";
		}
	}
}
