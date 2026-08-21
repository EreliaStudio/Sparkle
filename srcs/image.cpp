#include "image.hpp"

#include <limits>
#include <memory>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace spk
{
	Image::Format Image::_determineFormat(int channels)
	{
		switch (channels)
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

	Image::Image() :
		Texture(Target::Texture2D)
	{
	}

	Image::Image(std::span<const std::uint8_t> data) :
		Image()
	{
		_loadFromData(data);
	}

	void Image::_loadFromFile(const std::filesystem::path &path)
	{
		int width = 0;
		int height = 0;
		int channels = 0;

		stbi_uc *rawData = stbi_load(path.string().c_str(), &width, &height, &channels, 0);
		if (rawData == nullptr)
		{
			throw std::runtime_error("Image: failed to load file: " + path.string());
		}

		std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> imageData(rawData, stbi_image_free);
		const Format format = _determineFormat(channels);
		if (format == Format::Error)
		{
			throw std::runtime_error("Image: unsupported channel count");
		}

		setPixels(
			imageData.get(),
			Vector2UInt(static_cast<unsigned int>(width), static_cast<unsigned int>(height)),
			format);
		setMipmap(Mipmap::Enable);
		validate();
	}

	void Image::_loadFromData(std::span<const std::uint8_t> data)
	{
		if (data.empty())
		{
			throw std::invalid_argument("Image: encoded data cannot be empty");
		}
		if (data.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
		{
			throw std::overflow_error("Image: encoded data is too large for stb_image");
		}

		int width = 0;
		int height = 0;
		int channels = 0;

		stbi_uc *rawData = stbi_load_from_memory(
			reinterpret_cast<const stbi_uc *>(data.data()),
			static_cast<int>(data.size()),
			&width,
			&height,
			&channels,
			0);

		if (rawData == nullptr)
		{
			throw std::runtime_error("Image: failed to decode image data");
		}

		std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> imageData(rawData, stbi_image_free);
		const Format format = _determineFormat(channels);
		if (format == Format::Error)
		{
			throw std::runtime_error("Image: unsupported channel count");
		}

		setPixels(
			imageData.get(),
			Vector2UInt(static_cast<unsigned int>(width), static_cast<unsigned int>(height)),
			format);
		setMipmap(Mipmap::Enable);
		validate();
	}

	Image Image::open(const std::filesystem::path &path)
	{
		Image result;
		result._loadFromFile(path);
		return result;
	}
}
