#include "graphics/texture/spk_image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "external_libraries/stb_image.h"

namespace spk
{
	Image::Image(const std::filesystem::path& p_path) :
		_size(0),
		_channels(0),
		_textureData(nullptr),
		Texture()
	{
		if (!std::filesystem::exists(p_path))
		{
			throwException("File does not exist: " + p_path.string());
		}
		
		_textureData = stbi_load(p_path.string().c_str(), &_size.x, &_size.y, &_channels, 0);
		uploadToGPU(_textureData, _size, (_channels == 3 ? Texture::Format::RGB : Texture::Format::RGBA),
			Texture::Filtering::Nearest,
			Texture::Wrap::Repeat,
			Texture::Mipmap::Disable);
	}

	const spk::Vector2Int& Image::size() const
	{
		return (_size);
	}
}