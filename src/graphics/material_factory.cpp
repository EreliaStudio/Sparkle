#include "graphics/material.hpp"
#include "graphics/opengl/texture.hpp"
#include "graphics/sprite_sheet.hpp"

#include <cmath>
#include <string>

#include "asset/manager.hpp"
#include "container/json/reader.hpp"
#include "exception.hpp"

namespace spk
{
	namespace
	{
		using Reader = JSON::Reader;

		[[nodiscard]] Texture::Section readSection(const Reader &role, const SpriteSheet *sheet)
		{
			if (!role.contains("UV"))
			{
				return Texture::Section::whole;
			}
			const Reader uv = role.child("UV");
			uv.forbidUnknown({"sprite", "anchor", "size"});
			if (uv.contains("sprite"))
			{
				if (uv.contains("anchor") || uv.contains("size") || sheet == nullptr)
				{
					throw Exception(uv.pathFor("sprite") + ": requires only a SpriteSheet source");
				}
				const Vector2UInt coord = uv.require<Vector2UInt>(
					"sprite",
					[sheet](const Vector2UInt &value) {
						return value.x < sheet->nbSprite().x && value.y < sheet->nbSprite().y;
					},
					"coordinates out of grid");
				return sheet->sprite(coord);
			}
			const auto normalized = [](const TVector2<double> &value) {
				return std::isfinite(value.x) && std::isfinite(value.y) && value.x >= 0.0 && value.x <= 1.0 &&
					   value.y >= 0.0 && value.y <= 1.0;
			};
			const TVector2<double> anchor = uv.require<TVector2<double>>(
				"anchor",
				normalized,
				"expected finite coordinates in [0, 1]");
			const TVector2<double> size = uv.require<TVector2<double>>(
				"size",
				normalized,
				"expected finite coordinates in [0, 1]");
			if (size.x <= 0.0 || static_cast<float>(size.x) <= 0.0f || anchor.x + size.x > 1.0)
			{
				throw Exception(uv.pathFor("size") + ".x: invalid normalized width");
			}
			if (size.y <= 0.0 || static_cast<float>(size.y) <= 0.0f || anchor.y + size.y > 1.0)
			{
				throw Exception(uv.pathFor("size") + ".y: invalid normalized height");
			}
			return {Vector2(anchor), Vector2(size)};
		}

		void addRole(Material::Builder &builder, Asset::Manager &assets, const std::string &role, const Reader &entry)
		{
			entry.forbidUnknown({"Texture", "UV"});
			const Reader source = entry.child("Texture");
			source.forbidUnknown({"id", "path", "size"});

			const auto nonEmpty = [](const std::string &value) {
				return !value.empty();
			};
			const std::string path = source.require<std::string>("path", nonEmpty, "expected non-empty string");
			const std::string id = source.contains("id")
				? source.require<std::string>("id", nonEmpty, "expected non-empty string")
				: path;
			JSON::Value config = JSON::Value::object();
			config["path"] = path;

			if (source.contains("size"))
			{
				(void)source.require<Vector2UInt>(
					"size",
					[](const Vector2UInt &value) {
						return value.x > 0 && value.y > 0;
					},
					"expected positive grid dimensions");
				config["size"] = source.value().at("size");
				const SpriteSheet &sheet = assets.load<SpriteSheet>(id, config);
				builder.add(role, sheet, readSection(entry, &sheet));
				return;
			}

			const Texture &texture = assets.load<Texture>(id, config);
			builder.add(role, texture, readSection(entry, nullptr));
		}

		[[nodiscard]] Material readMaterial(Asset::Manager &assets, const JSON::Value &description)
		{
			const Reader root(description, {}, "material");
			root.forbidUnknown({"textures"});
			const Reader textures = root.child("textures");
			Material::Builder builder;
			for (const auto &[role, value] : textures.value().asObject())
			{
				if (role.empty() || role.find_first_not_of(" \t\n\r\f\v") == std::string::npos)
				{
					throw Exception(textures.pathFor(role) + ": invalid role name");
				}
				const Reader entry(value, {}, textures.pathFor(role));
				addRole(builder, assets, role, entry);
			}
			return builder.build();
		}
	}
}

namespace spk::Asset
{
	spk::Material Factory<spk::Material>::create(Manager &assets, const std::string &, const JSON::Value &config)
	{
		return spk::readMaterial(assets, config);
	}
}
