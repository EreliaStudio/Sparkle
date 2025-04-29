#include "structure/graphics/texture/spk_image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "external_libraries/stb_image.h"

#include "utils/spk_file_utils.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk
{
	Image::Image()
	{
	}

	Image::Image(const std::filesystem::path &p_path)
	{
		loadFromFile(p_path);
	}

	void Image::loadFromFile(const std::filesystem::path &p_path)
	{
		loadFromData(spk::FileUtils::readFileAsBytes(p_path));
	}

	void Image::loadFromData(const std::vector<uint8_t> &p_data)
	{
		int width, height, channels;
		stbi_uc *imageData =
			stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(p_data.data()), static_cast<int>(p_data.size()), &width, &height, &channels, 0);
		if (!imageData)
		{
			GENERATE_ERROR("Failed to load image from raw data.");
		}

		spk::Vector2UInt size{static_cast<unsigned int>(width), static_cast<unsigned int>(height)};
		Format format = _determineFormat(channels);

		setPixels(imageData, size, format, Filtering::Nearest, Wrap::ClampToEdge, Mipmap::Enable);

		stbi_image_free(imageData);
	}

	Texture::Format Image::_determineFormat(int p_channels) const
	{
		switch (p_channels)
		{
		case 1:
			return Format::GreyLevel;
		case 2:
			return Format::DualChannel;
		case 3:
			return Format::RGB;
		case 4:
			return Format::RGBA;
		default:
			return Format::Error;
		}
	}
}