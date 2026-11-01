#pragma once

#include <map>
#include <memory>
#include <string>
#include <string_view>

#include "graphics/sprite_sheet.hpp"

namespace spk
{
	class Material
	{
	public:
		struct Binding
		{
			Texture::Handle texture;
			Texture::Section section = Texture::Section::whole;
		};

		using Textures = std::map<std::string, Binding, std::less<>>;

		class Builder
		{
		private:
			Textures _textures;

		public:
			void add(const std::string &role, const Texture &texture, Texture::Section section = Texture::Section::whole);
			void add(const std::string &role, const SpriteSheet &sheet, const Vector2UInt &sprite);
			[[nodiscard]] Material build() const;
		};

	private:
		std::shared_ptr<const Textures> _textures;

		explicit Material(Textures textures);

	public:
		Material();

		[[nodiscard]] const Textures &textures() const noexcept;
		[[nodiscard]] const Binding *find(std::string_view role) const noexcept;
	};
}
