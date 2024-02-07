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
	 * @class VertexArrayObject
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
		/**
		 * @brief Default constructor.
		 * 
		 * Initializes a new Vertex Array Object (VAO) and generates a unique handle for it.
		 */
		VertexArrayObject();

		/**
		 * @brief Destructor.
		 * 
		 * Deletes the VAO and frees the associated OpenGL resource.
		 */
		~VertexArrayObject();

		/**
		 * @brief Activates the VAO.
		 * 
		 * Binds the VAO, making it the current vertex array object for OpenGL operations.
		 */
		void activate();

		/**
		 * @brief Deactivates the VAO.
		 * 
		 * Unbinds the VAO, resetting the current vertex array object to zero.
		 */
		void deactivate();

		/**
		 * @brief Retrieves the VAO handle.
		 * 
		 * @return The GLuint handle associated with this VAO.
		 */
		GLuint handle() const;

	};

    /**
	 * @class VertexBufferObject
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
		/**
		 * @brief Constructs a VertexBufferObject with a specified mode.
		 * 
		 * Initializes a new Vertex Buffer Object (VBO) and generates a unique handle for it. The mode specifies
		 * the buffer's intended usage (e.g., vertices, indices, or uniform block).
		 * 
		 * @param p_mode The mode of the VBO, determining its usage.
		 */
		VertexBufferObject(const Mode& p_mode);

		/**
		 * @brief Destructor.
		 * 
		 * Deletes the VBO and frees the associated OpenGL resource.
		 */
		~VertexBufferObject();

		/**
		 * @brief Move constructor.
		 * 
		 * Transfers the ownership of the VBO from another instance.
		 * 
		 * @param p_other The VertexBufferObject to move from.
		 */
		VertexBufferObject(VertexBufferObject&& p_other);

		/**
		 * @brief Move assignment operator.
		 * 
		 * Transfers the ownership of the VBO from another instance.
		 * 
		 * @param p_other The VertexBufferObject to move from.
		 * @return A reference to this VertexBufferObject.
		 */
		VertexBufferObject& operator=(VertexBufferObject&& p_other);

		/**
		 * @brief Activates the VBO.
		 * 
		 * Binds the VBO, making it the current buffer object for the specified mode.
		 */
		void activate();

		/**
		 * @brief Deactivates the VBO.
		 * 
		 * Unbinds the VBO for the specified mode, resetting the current buffer object to zero.
		 */
		void deactivate();

		/**
		 * @brief Uploads data to the VBO.
		 * 
		 * Copies the specified data to the VBO. The buffer is automatically resized to accommodate the data.
		 * 
		 * @param p_data Pointer to the data to upload.
		 * @param p_dataSize Size of the data in bytes.
		 */
		void push(const void* p_data, size_t p_dataSize);

		/**
		 * @brief Modifies a portion of the VBO's data.
		 * 
		 * Updates a specific portion of the buffer's data without affecting the rest of the buffer.
		 * 
		 * @param p_offset Offset in bytes to the start of the data to update.
		 * @param p_data Pointer to the new data.
		 * @param p_dataSize Size of the new data in bytes.
		 */
		void edit(size_t p_offset, const void* p_data, size_t p_dataSize);

		/**
		 * @brief Clears the VBO's data.
		 * 
		 * Resets the buffer's data, effectively setting its size to zero.
		 */
		void clear();

		/**
		 * @brief Retrieves the VBO handle.
		 * 
		 * @return The GLuint handle associated with this VBO.
		 */
		GLuint handle() const;

		/**
		 * @brief Retrieves the size of the VBO's data.
		 * 
		 * @return The size of the data currently stored in the VBO, in bytes.
		 */
		size_t size() const;
	};

    /**
	 * @class UniformBufferObject
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
		/**
		 * @brief Default constructor.
		 * 
		 * Initializes a new UniformBufferObject without associating it with any specific shader program or uniform block.
		 */
		UniformBufferObject();

		/**
		 * @brief Constructs a UniformBufferObject and associates it with a uniform block.
		 * 
		 * Initializes a new Uniform Buffer Object (UBO) and binds it to a specific uniform block in a shader program.
		 * 
		 * @param p_program The handle of the shader program containing the uniform block.
		 * @param p_uniformType The name of the uniform block within the shader program.
		 * @param p_blockBinding The binding point to use for this uniform block.
		 */
		UniformBufferObject(const GLuint& p_program, const std::string& p_uniformType, size_t p_blockBinding);

		/**
		 * @brief Activates the UBO.
		 * 
		 * Binds the UBO, making it the current uniform buffer object for its binding point.
		 */
		void activate();

		/**
		 * @brief Deactivates the UBO.
		 * 
		 * Unbinds the UBO for its binding point, resetting the current uniform buffer object to zero.
		 */
		void deactivate();
	};

    /**
	 * @class SamplerObject
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
		/**
		 * @brief Constructs a SamplerObject with a specified location.
		 * 
		 * Initializes a new SamplerObject and associates it with a sampler uniform location in a shader.
		 * 
		 * @param location The location of the sampler uniform in the shader program.
		 */
		SamplerObject(GLint location);

		/**
		 * @brief Assigns a texture unit to the sampler.
		 * 
		 * Sets the texture unit to be used by the sampler uniform. This method updates the sampler uniform to sample
		 * textures from the specified texture unit.
		 * 
		 * @param textureUnit The texture unit to bind to the sampler.
		 * @return A reference to this SamplerObject.
		 */
		SamplerObject& operator=(GLint textureUnit);

		/**
		 * @brief Activates the sampler.
		 * 
		 * Updates the shader program with the sampler's texture unit, preparing it for use in rendering.
		 */
		void activate();
    };
}