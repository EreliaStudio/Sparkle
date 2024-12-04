#pragma once

#include "structure/graphics/texture/spk_texture.hpp"
#include "structure/math/spk_vector2.hpp"
#include <filesystem>

namespace spk
{
	class Image : public Texture
	{
	public:
		Image();
		Image(const std::filesystem::path &p_path);

		void loadFromFile(const std::filesystem::path &p_path);
		void loadFromData(const std::vector<uint8_t> &p_data);

	private:
		Texture::Format _determineFormat(int p_channels) const;
	};
}