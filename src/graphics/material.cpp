#include "graphics/material.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <exception>
#include <utility>

#include "exception.hpp"

namespace spk
{
	namespace
	{
		[[nodiscard]] bool validRole(const std::string &role)
		{
			return !role.empty() && !std::all_of(role.begin(), role.end(), [](unsigned char character) {
				return std::isspace(character) != 0;
			});
		}

		[[nodiscard]] bool validSection(const Texture::Section &section)
		{
			const auto within = [](float value) {
				return std::isfinite(value) && value >= 0.0f && value <= 1.0f;
			};
			return within(section.anchor.x) && within(section.anchor.y) &&
				   within(section.size.x) && within(section.size.y) &&
				   section.size.x > 0.0f && section.size.y > 0.0f &&
				   section.anchor.x + section.size.x <= 1.0f &&
				   section.anchor.y + section.size.y <= 1.0f;
		}
	}

	Material::Material(Textures textures) :
		_textures(std::make_shared<const Textures>(std::move(textures)))
	{
	}

	Material::Material() :
		Material(Textures{})
	{
	}

	const Material::Textures &Material::textures() const noexcept
	{
		return *_textures;
	}

	const Material::Binding *Material::find(std::string_view role) const noexcept
	{
		const auto found = _textures->find(role);
		return found == _textures->end() ? nullptr : &found->second;
	}

	void Material::Builder::bind(Asset::Manager *assets)
	{
		if (assets == nullptr)
		{
			throw Exception("Material::Builder.bind: asset manager cannot be null");
		}
		if (_assets != nullptr && _assets != assets)
		{
			throw Exception("Material::Builder.bind: already bound to another asset manager");
		}
		_assets = assets;
	}

	void Material::Builder::add(const std::string &role, const Texture &texture, Texture::Section section)
	{
		if (!validRole(role))
		{
			throw Exception("Material: texture role cannot be empty or whitespace");
		}
		if (_textures.contains(role))
		{
			throw Exception("Material: duplicate texture role '" + role + "'");
		}
		if (!validSection(section))
		{
			throw Exception("Material: invalid normalized UV section for role '" + role + "'");
		}
		_textures.emplace(role, Binding{texture.handle(), section});
	}

	void Material::Builder::add(const std::string &role, const SpriteSheet &sheet, const Vector2UInt &sprite)
	{
		try
		{
			add(role, sheet, sheet.sprite(sprite));
		} catch (const std::out_of_range &)
		{
			throw Exception("Material: sprite coordinates out of range for role '" + role + "'", std::current_exception());
		}
	}

	Material Material::Builder::build() const
	{
		return Material(_textures);
	}
}
