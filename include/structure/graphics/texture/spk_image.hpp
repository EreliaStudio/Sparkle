#pragma once

#include <filesystem>
#include "structure/math/spk_vector2.hpp"
#include "structure/graphics/opengl/spk_texture_object.hpp"

namespace spk
{
	class Image : public OpenGL::TextureObject
	{
	public:
		struct Section
		{
			spk::Vector2 anchor;
			spk::Vector2 size;

			Section() = default;
			Section(spk::Vector2 p_anchor, spk::Vector2 p_size);
		};

	public:
		Image(const std::filesystem::path& filePath);

		void loadFromFile(const std::filesystem::path& filePath);

	private:
		Format _determineFormat(int channels) const;
	};
}