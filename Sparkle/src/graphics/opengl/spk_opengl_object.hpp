#pragma once

#include <GL/glew.h>
#include <GL/wglew.h>
#include <gl/GL.h>

#include <string>

namespace spk::OpenGL
{
	enum class Type
	{
		Byte = GL_BYTE,
		UnsignedByte = GL_UNSIGNED_BYTE,
		Short = GL_SHORT,
		UnsignedShort = GL_UNSIGNED_SHORT,
		Integer = GL_INT, 
		UnsignedInteger = GL_UNSIGNED_INT,
		Double = GL_DOUBLE,
		Float = GL_FLOAT
	};

	class VertexArrayObject
	{
	private:
		GLuint _handle;

	public:
		VertexArrayObject();
		~VertexArrayObject();

		void activate();
		void deactivate();

		GLuint handle() const;
	};

	class VertexBufferObject
	{
	public:
		enum class Mode
		{
			Error = GL_NONE,
			Vertices = GL_ARRAY_BUFFER,
			Indexes = GL_ELEMENT_ARRAY_BUFFER,
			UniformBlock = GL_UNIFORM_BUFFER
		};

	private:
		GLuint _handle;
		GLenum _mode;

		size_t _allocatedSize;
		size_t _size;

	public:
		VertexBufferObject(const Mode &p_mode);
		~VertexBufferObject();

		VertexBufferObject(const VertexBufferObject& p_other) = delete;
		VertexBufferObject& operator = (const VertexBufferObject& p_other) = delete;
		
		VertexBufferObject(VertexBufferObject&& p_other);
		VertexBufferObject& operator = (VertexBufferObject&& p_other);
		
		void activate();
		void deactivate();

		void push(const void*p_data, size_t p_dataSize);
		void edit(size_t p_offset, const void*p_data, size_t p_dataSize);

		void clear();

		GLuint handle() const;
		size_t size() const;
	};

	class UniformBufferObject : public VertexBufferObject
	{
	private:
		GLuint _blockIndex;
		GLuint _blockBinding;

	public:
		UniformBufferObject();
		UniformBufferObject(const GLuint& p_program, const std::string& p_uniformType, size_t p_blockBinding);
		
		void activate();
		void deactivate();
	};

	class SamplerObject
    {
    private:
        GLint _location;
        GLint _textureUnit;

    public:
        SamplerObject(GLint location);

        SamplerObject& operator=(GLint textureUnit);

        void activate();
    };
}