#pragma once

#include <cstdint>
#include <filesystem>
#include <span>

#include "texture.hpp"

namespace spk
{
	class Image : public Texture
	{
	private:
		[[nodiscard]] static Format _determineFormat(int channels);

	protected:
		Image();

		void _loadFromFile(const std::filesystem::path &path);
		void _loadFromData(std::span<const std::uint8_t> data);

	public:
		explicit Image(std::span<const std::uint8_t> data);

		[[nodiscard]] static Image open(const std::filesystem::path &path);
	};
}
