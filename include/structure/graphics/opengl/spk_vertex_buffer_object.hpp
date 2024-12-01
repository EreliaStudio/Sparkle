#pragma once

#include <GL/glew.h>
#include <GL/gl.h>
#include "structure/container/spk_data_buffer.hpp"

namespace spk::OpenGL
{
	class VertexBufferObject
	{
	public:
		enum class Type : GLenum
		{
			Unknow = GL_INVALID_ENUM,
			Storage = GL_ARRAY_BUFFER,
			Layout = GL_ELEMENT_ARRAY_BUFFER,
			Uniform = GL_UNIFORM_BUFFER,
			Texture = GL_TEXTURE_BUFFER,
			TransformFeedback = GL_TRANSFORM_FEEDBACK_BUFFER,
			ShaderStorage = GL_SHADER_STORAGE_BUFFER,
			PixelPack = GL_PIXEL_PACK_BUFFER,
			PixelUnpack = GL_PIXEL_UNPACK_BUFFER,
			DrawIndirect = GL_DRAW_INDIRECT_BUFFER,
			AtomicCounter = GL_ATOMIC_COUNTER_BUFFER
		};

		enum class Usage : GLenum
		{
			Unknow = GL_INVALID_ENUM,
			Static = GL_STATIC_DRAW,
			Dynamic = GL_DYNAMIC_DRAW,
			Stream = GL_STREAM_DRAW,
			StaticRead = GL_STATIC_READ,
			DynamicRead = GL_DYNAMIC_READ,
			StreamRead = GL_STREAM_READ,
			StaticCopy = GL_STATIC_COPY,
			DynamicCopy = GL_DYNAMIC_COPY,
			StreamCopy = GL_STREAM_COPY
		};

	protected:
		GLuint _id = 0;

	private:
		bool _validated = false;
		Type _type = Type::Unknow;
		Usage _usage = Usage::Unknow;
		spk::DataBuffer _buffer;
		size_t _currentBufferMaxSize = 0;

		void _allocate();
		void _release();
		void _update();

	public:
		VertexBufferObject();
		VertexBufferObject(Type p_type, Usage p_usage);
		~VertexBufferObject();

		VertexBufferObject(const VertexBufferObject& p_other);
		VertexBufferObject(VertexBufferObject&& p_other) noexcept;

		VertexBufferObject& operator=(const VertexBufferObject& p_other);
		VertexBufferObject& operator=(VertexBufferObject&& p_other) noexcept;

		virtual void activate();
		void deactivate();

		void clear();
		void resize(size_t p_size);
		void* data();
		size_t size() const;
		void edit(const void* p_data, size_t p_dataSize, size_t p_offset);
		void append(const void* p_data, size_t p_dataSize);
		void validate();
	};
}
