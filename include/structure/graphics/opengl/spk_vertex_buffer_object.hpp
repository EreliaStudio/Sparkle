#pragma once

#include <GL/glew.h>
#include <GL/gl.h>
#include "structure/container/spk_data_buffer.hpp"

namespace spk::OpenGL
{
	/**
	 * @class VertexBufferObject
	 * @brief Encapsulates an OpenGL Vertex Buffer Object (VBO) for efficient GPU data management.
	 * 
	 * The `VertexBufferObject` class provides functionality to manage OpenGL buffer objects, including creation,
	 * resizing, data editing, and activation. It supports various buffer types and usage patterns.
	 * 
	 * Example usage:
	 * @code
	 * spk::OpenGL::VertexBufferObject vbo(spk::OpenGL::VertexBufferObject::Type::Storage,
	 *									  spk::OpenGL::VertexBufferObject::Usage::Static);
	 * 
	 * vbo.resize(1024); // Allocate 1024 bytes
	 * vbo.activate();   // Bind the buffer
	 * vbo.append(data, size); // Append data
	 * vbo.validate();   // Upload data to GPU
	 * vbo.deactivate(); // Unbind the buffer
	 * @endcode
	 */
	class VertexBufferObject
	{
	public:
		/**
		 * @brief Enumerates the types of OpenGL buffer objects.
		 */
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

		/**
		 * @brief Enumerates the usage patterns for OpenGL buffer objects.
		 */
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
		GLuint _id = 0; /**< @brief OpenGL ID of the buffer. */

	private:
		bool _validated = false; /**< @brief Indicates if the buffer is validated and uploaded to the GPU. */
		Type _type = Type::Unknow; /**< @brief The type of the buffer. */
		Usage _usage = Usage::Unknow; /**< @brief The usage pattern of the buffer. */
		spk::DataBuffer _buffer; /**< @brief Internal buffer for managing data. */
		size_t _currentBufferMaxSize = 0; /**< @brief The current allocated size of the buffer. */

		/**
		 * @brief Allocates the OpenGL buffer.
		 */
		void _allocate();

		/**
		 * @brief Releases the OpenGL buffer.
		 */
		void _release();

		/**
		 * @brief Updates the GPU buffer with the current data.
		 */
		void _update();

	public:
		/**
		 * @brief Default constructor.
		 */
		VertexBufferObject();

		/**
		 * @brief Constructs a VertexBufferObject with a specific type and usage.
		 * 
		 * @param p_type The type of the buffer.
		 * @param p_usage The usage pattern of the buffer.
		 */
		VertexBufferObject(Type p_type, Usage p_usage);

		/**
		 * @brief Destructor. Releases the OpenGL buffer if allocated.
		 */
		~VertexBufferObject();

		VertexBufferObject(const VertexBufferObject& p_other); /**< @brief Copy constructor. */
		VertexBufferObject(VertexBufferObject&& p_other) noexcept; /**< @brief Move constructor. */

		VertexBufferObject& operator=(const VertexBufferObject& p_other); /**< @brief Copy assignment operator. */
		VertexBufferObject& operator=(VertexBufferObject&& p_other) noexcept; /**< @brief Move assignment operator. */

		/**
		 * @brief Activates (binds) the buffer to the current OpenGL context.
		 */
		virtual void activate();

		/**
		 * @brief Deactivates (unbinds) the buffer from the OpenGL context.
		 */
		void deactivate();

		/**
		 * @brief Clears the internal data buffer.
		 */
		void clear();

		/**
		 * @brief Resizes the internal buffer to the specified size.
		 * 
		 * @param p_size The new size of the buffer in bytes.
		 */
		void resize(size_t p_size);

		/**
		 * @brief Retrieves a pointer to the internal data buffer.
		 * 
		 * @return A void pointer to the buffer data.
		 */
		void* data();

		/**
		 * @brief Gets the current size of the internal buffer.
		 * 
		 * @return The size of the buffer in bytes.
		 */
		size_t size() const;

		/**
		 * @brief Edits a portion of the buffer with new data.
		 * 
		 * @param p_data Pointer to the data to write.
		 * @param p_dataSize The size of the data in bytes.
		 * @param p_offset The offset in the buffer to start writing to.
		 */
		void edit(const void* p_data, size_t p_dataSize, size_t p_offset);

		/**
		 * @brief Appends data to the buffer.
		 * 
		 * @param p_data Pointer to the data to append.
		 * @param p_dataSize The size of the data in bytes.
		 */
		void append(const void* p_data, size_t p_dataSize);

		/**
		 * @brief Validates and uploads the buffer to the GPU.
		 */
		void validate();
	};
}
