#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>

#include "graphics/opengl/texture.hpp"

namespace spk
{
	class Image : public Texture
	{
	private:
		[[nodiscard]] static Format _determineFormat(int channels);

	protected:
		static void _validateEncodedDataSize(std::size_t size);

		void _loadFromFile(const std::filesystem::path &path);
		void _loadFromData(std::span<const std::uint8_t> data);

	public:
		Image();
		explicit Image(std::span<const std::uint8_t> data);
		[[nodiscard]] std::unique_ptr<GPUResource> clone() const override
		{
			auto result = std::make_unique<Image>(*this);
			result->_setState(_cloneState());
			return result;
		}

		[[nodiscard]] static Image open(const std::filesystem::path &path);
	};
}
