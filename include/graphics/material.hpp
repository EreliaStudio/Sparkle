#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>

#include "asset/factory.hpp"
#include "graphics/sprite_sheet.hpp"
#include "rendering/engine/engine_view.hpp"

namespace spk
{
	class Material
	{
	public:
		struct Binding
		{
			Texture::Handle texture;
			Texture::Section section = Texture::Section::whole;
			std::weak_ptr<EngineView::Output> viewOutput;
			std::uint64_t viewIdentity = 0;

			[[nodiscard]] bool isViewSource() const noexcept
			{
				return viewIdentity != 0;
			}

			[[nodiscard]] bool valid() const noexcept
			{
				return isViewSource() ? !viewOutput.expired() : texture.valid();
			}

			[[nodiscard]] bool sameSource(const Binding &other) const noexcept
			{
				if (isViewSource() != other.isViewSource())
				{
					return false;
				}
				if (!isViewSource())
				{
					return texture.identifier() == other.texture.identifier();
				}
				return viewIdentity == other.viewIdentity &&
					   !viewOutput.owner_before(other.viewOutput) &&
					   !other.viewOutput.owner_before(viewOutput);
			}
		};

		using Textures = std::map<std::string, Binding, std::less<>>;

		class Builder
		{
		private:
			Textures _textures;

		public:
			void add(const std::string &role, const Texture &texture, Texture::Section section = Texture::Section::whole);
			void add(const std::string &role, const EngineView &view, Texture::Section section = Texture::Section::whole);
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

namespace spk::Asset
{
	template <>
	struct Factory<spk::Material>
	{
		[[nodiscard]] static spk::Material create(Manager &assets, const std::string &id, const JSON::Value &config);
	};
}
