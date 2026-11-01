#pragma once

#include <map>
#include <memory>
#include <string>
#include <string_view>

#include "asset/manager.hpp"
#include "container/json/object.hpp"
#include "graphics/sprite_sheet.hpp"

namespace spk::JSON
{
	class Reader;
}

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
			Assets *_assets = nullptr;

			[[nodiscard]] Binding _readRole(const JSON::Reader &reader) const;

		public:
			void bind(Assets *assets);
			void add(const std::string &role, const Texture &texture, Texture::Section section = Texture::Section::whole);
			void add(const std::string &role, const SpriteSheet &sheet, const Vector2UInt &sprite);
			void load(const JSON::Value &description);
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

namespace spk
{
	template <>
	struct AssetFactory<spk::Texture>
	{
		[[nodiscard]] static spk::Texture create(Assets &assets, const std::string &id, const JSON::Value &config);
	};

	template <>
	struct AssetFactory<spk::SpriteSheet>
	{
		[[nodiscard]] static spk::SpriteSheet create(Assets &assets, const std::string &id, const JSON::Value &config);
	};

	template <>
	struct AssetFactory<spk::Material>
	{
		[[nodiscard]] static spk::Material create(Assets &assets, const std::string &id, const JSON::Value &config);
	};
}
