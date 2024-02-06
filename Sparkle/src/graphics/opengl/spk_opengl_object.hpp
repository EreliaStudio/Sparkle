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

    /**
     * @brief Encapsulates an OpenGL Vertex Array Object (VAO), managing the state needed for rendering.
     *
     * VertexArrayObject simplifies the creation, binding, and deletion of VAOs, which are essential for modern OpenGL rendering. VAOs store
     * the state needed to supply vertex data to the GPU, including bindings for vertex buffer objects (VBOs) and attribute settings.
     *
     * Usage example:
     * @code
     * VertexArrayObject vao;
     * vao.activate();
     * // Setup your VBOs and attribute pointers here
     * vao.deactivate();
     * @endcode
	 * 
	 * @note This class isn't suppose to be instanciated by the user, the documentation is provided only for informative propose.
     */
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

    /**
     * @brief Wraps an OpenGL Vertex Buffer Object (VBO), used for storing vertex data or indices in GPU memory.
     *
     * VertexBufferObject manages the lifecycle of a VBO, including its creation, data storage, and cleanup. VBOs are used to efficiently
     * store data such as vertex coordinates, normals, texture coordinates, or element indices in the GPU, reducing the need for frequent data transfers.
     *
     * The `Mode` enum specifies the buffer's purpose, supporting vertices, indices, and uniform blocks.
     *
     * Usage example:
     * @code
     * VertexBufferObject vbo(VertexBufferObject::Mode::Vertices);
     * vbo.activate();
     * vbo.push(vertices.data(), vertices.size() * sizeof(vertices[0]));
     * vbo.deactivate();
     * @endcode
	 * 
	 * @note This class isn't suppose to be instanciated by the user, the documentation is provided only for informative propose.
     */
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

    /**
     * @brief Specializes VertexBufferObject for OpenGL Uniform Buffer Objects (UBOs), facilitating shader uniform management.
     *
     * UniformBufferObject provides an interface for creating, binding, and managing UBOs. UBOs allow for efficient sharing of uniform data
     * across multiple shaders, enabling consistent and easy-to-update shader configurations.
     *
     * Usage example:
     * @code
     * UniformBufferObject ubo(shaderProgram, "GlobalMatrices", 0);
     * ubo.activate();
     * ubo.push(&matrices, sizeof(matrices));
     * ubo.deactivate();
     * @endcode
	 * 
	 * @note This class isn't suppose to be instanciated by the user, the documentation is provided only for informative propose.
     */
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

    /**
     * @brief Manages OpenGL sampler uniforms, linking texture units to shader sampler locations.
     *
     * SamplerObject abstracts the association between a shader's sampler uniform and an OpenGL texture unit, facilitating texture binding
     * and activation within shaders. It simplifies the process of setting which texture unit a shader's sampler should use.
     *
     * Usage example:
     * @code
     * SamplerObject sampler(shader.getUniformLocation("myTextureSampler"));
     * sampler = 0; // Bind the sampler to texture unit 0
     * sampler.activate();
     * @endcode
	 * 
	 * @note This class isn't suppose to be instanciated by the user, the documentation is provided only for informative propose.
     */
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