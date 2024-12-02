#pragma once

#include <GL/glew.h>
#include <GL/gl.h>
#include <vector>
#include <cstring>
#include "structure/graphics/spk_geometry_2D.hpp"

namespace spk::OpenGL
{
	/**
	 * @brief Represents a texture object in OpenGL.
	 *
	 * This class encapsulates the creation, setup, and management of OpenGL textures,
	 * including data upload and property configuration.
	 *
	 * Example usage:
	 * @code
	 * spk::OpenGL::TextureObject texture;
	 * texture.setData(data, size, spk::OpenGL::TextureObject::Format::RGBA);
	 * texture.setProperties(spk::OpenGL::TextureObject::Filtering::Linear,
	 *					   spk::OpenGL::TextureObject::Wrap::Repeat,
	 *					   spk::OpenGL::TextureObject::Mipmap::Enable);
	 * @endcode
	 */
	class TextureObject
	{
		friend class SamplerObject;
		friend class FrameBufferObject;

	public:
		/**
		 * @brief Enum representing the texture format.
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
		 * @brief Enum representing the texture filtering mode.
		 */
		enum class Filtering
		{
			Nearest = GL_NEAREST,
			Linear = GL_LINEAR
		};

		/**
		 * @brief Enum representing the texture wrapping mode.
		 */
		enum class Wrap
		{
			Repeat = GL_REPEAT,
			ClampToEdge = GL_CLAMP_TO_EDGE,
			ClampToBorder = GL_CLAMP_TO_BORDER
		};

		/**
		 * @brief Enum to enable or disable mipmapping.
		 */
		enum class Mipmap
		{
			Disable,
			Enable
		};

	private:
		GLuint _id;
		bool _ownTexture;
		bool _needUpload = false;
		bool _needSetup = false;
		std::vector<uint8_t> _datas;
		spk::Vector2UInt _size;
		Format _format;
		Filtering _filtering;
		Wrap _wrap;
		Mipmap _mipmap;

		/**
		 * @brief Uploads texture data to the GPU.
		 */
		void _upload();

		/**
		 * @brief Sets up the texture properties.
		 */
		void _setup();

		/**
		 * @brief Gets the number of bytes per pixel for a given format.
		 * @param format The texture format.
		 * @return Number of bytes per pixel.
		 */
		size_t _getBytesPerPixel(const Format& format) const;

	public:
		/**
		 * @brief Default constructor.
		 */
		TextureObject();
		
		/**
		 * @brief Copy constructor.
		 * @param other The TextureObject to copy from.
		 */
		TextureObject(const TextureObject& other);

		/**
		 * @brief Copy assignment operator.
		 * @param other The TextureObject to copy from.
		 * @return Reference to the current object.
		 */
		TextureObject& operator=(const TextureObject& other);

		/**
		 * @brief Move constructor.
		 * @param p_other The TextureObject to move from.
		 */
		TextureObject(TextureObject&& p_other) noexcept;

		/**
		 * @brief Move assignment operator.
		 * @param p_other The TextureObject to move from.
		 * @return Reference to the current object.
		 */
		TextureObject& operator=(TextureObject&& p_other) noexcept;

		/**
		 * @brief Destructor. Releases the OpenGL texture.
		 */
		virtual ~TextureObject();

		/**
		 * @brief Sets the texture data and properties.
		 * @param p_data Raw texture data.
		 * @param p_size Dimensions of the texture.
		 * @param p_format Format of the texture.
		 * @param p_filtering Filtering mode.
		 * @param p_wrap Wrapping mode.
		 * @param p_mipmap Mipmapping configuration.
		 */
		void setData(const std::vector<uint8_t>& p_data, const spk::Vector2UInt& p_size,
			const Format& p_format, const Filtering& p_filtering,
			const Wrap& p_wrap, const Mipmap& p_mipmap);

		/**
		 * @brief Sets the texture data with default properties.
		 * @param p_data Raw texture data.
		 * @param p_size Dimensions of the texture.
		 * @param p_format Format of the texture.
		 */
		void setData(const std::vector<uint8_t>& p_data, const spk::Vector2UInt& p_size, const Format& p_format);

		/**
		 * @brief Sets the texture data and properties.
		 * @param p_data Pointer to raw texture data.
		 * @param p_size Dimensions of the texture.
		 * @param p_format Format of the texture.
		 * @param p_filtering Filtering mode.
		 * @param p_wrap Wrapping mode.
		 * @param p_mipmap Mipmapping configuration.
		 */
		void setData(const uint8_t* p_data, const spk::Vector2UInt& p_size,
			const Format& p_format, const Filtering& p_filtering,
			const Wrap& p_wrap, const Mipmap& p_mipmap);

		/**
		 * @brief Sets the texture data with default properties.
		 * @param p_data Pointer to raw texture data.
		 * @param p_size Dimensions of the texture.
		 * @param p_format Format of the texture.
		 */
		void setData(const uint8_t* p_data, const spk::Vector2UInt& p_size, const Format& p_format);

		/**
		 * @brief Configures the texture properties.
		 * @param p_filtering Filtering mode.
		 * @param p_wrap Wrapping mode.
		 * @param p_mipmap Mipmapping configuration.
		 */
		void setProperties(const Filtering& p_filtering, const Wrap& p_wrap, const Mipmap& p_mipmap);

		/**
		 * @brief Retrieves the raw texture data.
		 * @return Reference to the raw texture data.
		 */
		const std::vector<uint8_t>& data() const;

		/**
		 * @brief Retrieves the texture dimensions.
		 * @return Dimensions of the texture.
		 */
		spk::Vector2UInt size() const;

		/**
		 * @brief Retrieves the texture format.
		 * @return Format of the texture.
		 */
		Format format() const;

		/**
		 * @brief Retrieves the texture filtering mode.
		 * @return Filtering mode of the texture.
		 */
		Filtering filtering() const;

		/**
		 * @brief Retrieves the texture wrapping mode.
		 * @return Wrapping mode of the texture.
		 */
		Wrap wrap() const;

		/**
		 * @brief Retrieves the mipmapping configuration.
		 * @return Mipmapping configuration.
		 */
		Mipmap mipmap() const;
	};
}
