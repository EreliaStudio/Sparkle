#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <GL/glew.h>
#include <GL/wglew.h>
#include <gl/GL.h>

#include "math/spk_vector2.hpp"

namespace spk
{
	/**
	 * @class Texture
	 * @brief Manages texture data for use in rendering pipelines.
	 *
	 * This class encapsulates the functionality required to handle texture images, including loading them
	 * into GPU memory, setting their format, filtering modes, wrapping modes, and managing mipmaps.
	 * It supports various formats for texture data, such as RGB, RGBA, and greyscale, and provides options
	 * for how textures are sampled and how their edges are treated.
	 *
	 * Textures are essential for adding detail to rendered objects by applying image data to their
	 * surfaces. This class provides a straightforward interface for creating textures from image data,
	 * configuring their properties, and binding them for use in rendering operations.
	 *
	 * Usage example:
	 * @code
	 * spk::Texture texture(imageData, imageSize, spk::Texture::Format::RGBA, 
	 *					  spk::Texture::Filtering::Linear, spk::Texture::Wrap::Repeat, 
	 *					  spk::Texture::Mipmap::Enable);
	 * texture.uploadToGPU();
	 * @endcode
	 *
	 * @note Before a texture can be used in rendering, it must be uploaded to GPU memory using the `uploadToGPU` method.
	 * @note If data have been pushed to the GPU, it will be automaticaly release uppon destructor call or uppon re-upload to GPU.
	 *
	 * @see Pipeline, Vector2UInt
	 */
	class Texture
	{
	public:
		/**
		 * @enum Format
		 * @brief Defines the pixel format of the texture.
		 *
		 * This enumeration specifies the format of the pixel data in the texture. Different formats represent how the pixel data is laid out in memory and how it should be interpreted when the texture is used for rendering.
		 *
		 * - RGB: Standard RGB color space with three components per pixel.
		 * - RGBA: Standard RGB color space but with an additional alpha channel for transparency.
		 * - BGR: Reverse of RGB, often used in certain systems or image formats.
		 * - BGRA: Reverse of RGBA, including an alpha channel for transparency.
		 * - GreyLevel: Single channel format for grayscale images.
		 * - Error: Represents an error state or undefined pixel format.
		 */
		enum class Format
		{
			RGB = GL_RGB,
			RGBA = GL_RGBA,
			BGR = GL_BGR,
			BGRA = GL_BGRA,
			GreyLevel = GL_RED,
			DualChannel = GL_RG,
			Error = GL_NONE
		};

		/**
		 * @enum Filtering
		 * @brief Defines texture filtering modes for magnification and minification.
		 *
		 * Texture filtering affects how textures are sampled when they are mapped to surfaces that are larger or smaller than the texture itself.
		 *
		 * - Nearest: Uses the nearest pixel's color value, resulting in a pixelated look.
		 * - Linear: Averages the colors of the nearest pixels, resulting in a smoother appearance.
		 */
		enum class Filtering
		{
			Nearest = GL_NEAREST,
			Linear = GL_LINEAR
		};

		/**
		 * @enum Wrap
		 * @brief Specifies how textures are sampled outside their standard 0 to 1 UV coordinate range.
		 *
		 * Defines the behavior of texture sampling in the S and T directions (equivalent to X and Y) when texture coordinates fall outside the range of [0, 1].
		 *
		 * - Repeat: The texture is repeated outside of the [0, 1] range.
		 * - ClampToEdge: Coordinates outside [0, 1] are clamped to the nearest edge's color.
		 * - ClampToBorder: Coordinates outside [0, 1] are given a border color.
		 */
		enum class Wrap
		{
			Repeat = GL_REPEAT,
			ClampToEdge = GL_CLAMP_TO_EDGE,
			ClampToBorder = GL_CLAMP_TO_BORDER
		};

		/**
		 * @enum Mipmap
		 * @brief Determines whether mipmaps should be generated for the texture.
		 *
		 * Mipmaps are pre-calculated, optimized sequences of images that accompany a main texture, intended to increase rendering speed and reduce aliasing artifacts. They are particularly useful for textures viewed at a distance.
		 *
		 * - Disable: Mipmaps are not used.
		 * - Enable: Mipmaps are generated and used for the texture.
		 */
		enum class Mipmap
		{
			Disable,
			Enable
		};
		
	private:
		bool _loaded;
		GLuint _textureID;

		friend class Pipeline;

		void _bind(int p_textureIndex = 0) const;
		void _unbind() const;

	public:
		/**
		 * @brief Default constructor for the Texture class.
		 * 
		 * Initializes a Texture object without any image data. Use `uploadToGPU` to load texture data into GPU memory.
		 */
		Texture();

		/**
		 * @brief Constructs a Texture with specified parameters and prepares it for GPU upload.
		 * 
		 * This constructor initializes a Texture object with the provided image data and configuration parameters, including format, filtering, wrap mode, and mipmap generation. However, it does not automatically upload the texture data to the GPU; use `uploadToGPU` for that purpose.
		 * 
		 * @param p_textureData Pointer to the raw texture data.
		 * @param p_textureSize Dimensions of the texture (width and height) as a Vector2UInt.
		 * @param p_format Format of the texture data (e.g., RGB, RGBA).
		 * @param p_filtering Filtering mode to use for the texture (e.g., Nearest, Linear).
		 * @param p_wrap Wrap mode for the texture (e.g., Repeat, ClampToEdge).
		 * @param p_mipmap Specifies whether to generate mipmaps (Enable or Disable).
		 */
		Texture(const uint8_t* p_textureData, const spk::Vector2UInt& p_textureSize,
			const Format& p_format, const Filtering& p_filtering,
			const Wrap& p_wrap, const Mipmap& p_mipmap);

		/**
		 * @brief Destructor for the Texture class.
		 * 
		 * Cleans up resources associated with the Texture object. If the texture has been uploaded to the GPU, it ensures that the corresponding GPU memory is released.
		 */
		virtual ~Texture();

		/**
		 * @brief Copy constructor (deleted).
		 * 
		 * Prevents the creation of a Texture instance by copying from another Texture instance. This deletion ensures that each Texture object maintains unique ownership of its GPU resources. Copying Texture objects could lead to issues like double deletion of resources, hence it is explicitly deleted to avoid such problems.
		 * 
		 * @param p_other The other Texture instance to copy from.
		 */
		Texture(const Texture& p_other) = delete;

		/**
		 * @brief Copy assignment operator (deleted).
		 * 
		 * Disables the assignment of one Texture object to another through copying. This operation is deleted to prevent the inadvertent copying of GPU resource ownership, which could lead to resource management issues such as double frees. Texture objects should be uniquely owned and managed to ensure resource integrity.
		 * 
		 * @param p_other The other Texture instance to assign from.
		 * @return A reference to this Texture instance after deletion.
		 */
		Texture& operator = (const Texture& p_other) = delete;

		/**
		 * @brief Move constructor.
		 * 
		 * Transfers the ownership of the GPU resources from another Texture instance to this instance. This constructor is used to efficiently move a Texture object, including its GPU resources, without the overhead of copying the texture data. It leaves the moved-from object in a valid but unspecified state.
		 * 
		 * @param p_other The other Texture instance to move from.
		 */
		Texture(Texture&& p_other);

		/**
		 * @brief Move assignment operator.
		 * 
		 * Enables the assignment of a Texture object from another Texture object using move semantics. This operation transfers the GPU resources ownership from one Texture to another, effectively transferring the texture data and leaving the moved-from object in a valid but unspecified state. It ensures efficient resource management and avoids the duplication of GPU resources.
		 * 
		 * @param p_other The other Texture instance to move from.
		 * @return A reference to this Texture instance after acquiring the resources.
		 */
		Texture& operator = (Texture&& p_other);

		/**
		 * @brief Uploads texture data to GPU memory.
		 * 
		 * This method uploads the provided image data to GPU memory according to the specified format, filtering mode, wrap mode, and mipmap generation option. It is responsible for creating a GPU texture object and configuring it with the provided parameters.
		 * 
		 * @param p_textureData Pointer to the raw texture data.
		 * @param p_textureSize Dimensions of the texture (width and height) as a Vector2UInt.
		 * @param p_format Format of the texture data (e.g., RGB, RGBA).
		 * @param p_filtering Filtering mode to use for the texture (e.g., Nearest, Linear).
		 * @param p_wrap Wrap mode for the texture (e.g., Repeat, ClampToEdge).
		 * @param p_mipmap Specifies whether to generate mipmaps (Enable or Disable).
		 */
		void uploadToGPU(const uint8_t* p_textureData, const spk::Vector2UInt& p_textureSize,
			const Format& p_format, const Filtering& p_filtering,
			const Wrap& p_wrap, const Mipmap& p_mipmap);

		/**
		 * @brief Releases the texture's GPU memory.
		 * 
		 * If the texture has been uploaded to the GPU, this method releases the allocated GPU memory. This is useful for managing resources explicitly, especially in applications where textures are frequently created and destroyed.
		 */
		void releaseGPUMemory();

		/**
		 * @brief Return the ID provided by sparkle when creating/uploading the texture to the GPU
		 * @return The ID of the texture allocated on the GPU
		*/
		const GLint ID() const;
	};
}
