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
			union
			{
				spk::Vector2 anchor;
				struct
				{
					float x;
					float y;
				};
			};

			union
			{
				spk::Vector2 size;
				struct
				{
					float width;
					float height;
				};
			};

			Section();
			Section(spk::Vector2 p_anchor, spk::Vector2 p_size);
		};

	public:
		Image(const std::filesystem::path& filePath);

		void loadFromFile(const std::filesystem::path& filePath);

	private:
		Format _determineFormat(int channels) const;
	};
}