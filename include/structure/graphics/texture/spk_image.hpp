#pragma once

#include <filesystem>
#include "structure/math/spk_vector2.hpp"
#include "structure/graphics/texture/spk_texture.hpp"

namespace spk
{
	class Image : public Texture
	{
	public:
		Image();
		Image(const std::filesystem::path& p_path);

		void loadFromFile(const std::filesystem::path& p_path);
		void loadFromData(const std::vector<uint8_t>& p_data);

	private:
		Format _determineFormat(int channels) const;
	};
}