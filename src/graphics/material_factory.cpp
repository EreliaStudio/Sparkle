#include "graphics/material.hpp"
#include "graphics/opengl/texture.hpp"
#include "graphics/sprite_sheet.hpp"

#include <cmath>
#include <exception>
#include <string>

#include "container/json/reader.hpp"
#include "exception.hpp"

namespace spk
{
	namespace
	{
		using Reader = JSON::Reader;

		[[nodiscard]] std::string nonEmptyString(const Reader &reader, const std::string &key)
		{
			const std::string value = reader.require<std::string>(key);
			if (value.empty())
			{
				throw Exception(reader.pathFor(key) + ": expected non-empty string");
			}
			return value;
		}

		[[nodiscard]] Reader vectorMember(const Reader &reader, const std::string &key)
		{
			if (!reader.contains(key))
			{
				throw Exception(reader.pathFor(key) + ": missing coordinates");
			}
			return Reader(reader.value().at(key), {}, reader.pathFor(key));
		}

		[[nodiscard]] Vector2UInt grid(const Reader &reader, bool allowZero)
		{
			Vector2UInt value;
			try
			{
				value = reader.value().as<Vector2UInt>();
			} catch (...)
			{
				throw Exception(reader.path() + ": expected two unsigned coordinates", std::current_exception());
			}
			if (!allowZero && value.x == 0)
			{
				throw Exception(reader.pathFor("x") + ": expected positive integer");
			}
			if (!allowZero && value.y == 0)
			{
				throw Exception(reader.pathFor("y") + ": expected positive integer");
			}
			return value;
		}

		[[nodiscard]] TVector2<double> normalizedPoint(const Reader &reader)
		{
			TVector2<double> value;
			try
			{
				value = reader.value().as<TVector2<double>>();
			} catch (...)
			{
				throw Exception(reader.path() + ": expected two numeric coordinates", std::current_exception());
			}
			if (!std::isfinite(value.x) || value.x < 0.0 || value.x > 1.0)
			{
				throw Exception(reader.pathFor("x") + ": expected finite number in [0, 1]");
			}
			if (!std::isfinite(value.y) || value.y < 0.0 || value.y > 1.0)
			{
				throw Exception(reader.pathFor("y") + ": expected finite number in [0, 1]");
			}
			return value;
		}

		void validateConfiguration(const std::string &type, const JSON::Value &config, const std::string &path)
		{
			Reader reader(config, {}, path);
			reader.forbidUnknown(type == "Texture" ? std::initializer_list<std::string_view>{"path"} :
												  std::initializer_list<std::string_view>{"path", "size"});
			(void)nonEmptyString(reader, "path");
			if (type == "SpriteSheet")
			{
				(void)grid(vectorMember(reader, "size"), false);
			}
		}

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
				const Vector2UInt coord = grid(vectorMember(uv, "sprite"), true);
				try
				{
					return sheet->sprite(coord);
				} catch (...)
				{
					throw Exception(uv.pathFor("sprite") + ": coordinates out of grid", std::current_exception());
				}
			}
			if (!uv.contains("anchor") || !uv.contains("size"))
			{
				throw Exception(uv.path() + ": expected anchor and size, or sprite");
			}
			const TVector2<double> anchor = normalizedPoint(vectorMember(uv, "anchor"));
			const TVector2<double> size = normalizedPoint(vectorMember(uv, "size"));
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
		void addLoadedRole(Material::Builder &builder, Asset::Manager &assets, const std::string &role,
						   const Reader &entry, const std::string &type, const std::string &id, const JSON::Value &config)
		{
			if (type == "SpriteSheet")
			{
				const SpriteSheet &sheet = assets.load<SpriteSheet>(id, config);
				builder.add(role, sheet, readSection(entry, &sheet));
			}
			else
			{
				const Texture &texture = assets.load<Texture>(id, config);
				builder.add(role, texture, readSection(entry, nullptr));
			}
		}

		void addPathRole(Material::Builder &builder, Asset::Manager &assets, const std::string &role,
						 const Reader &entry, const Reader &source)
		{
			const std::string path = nonEmptyString(source, "path");
			JSON::Value config = JSON::Value::object();
			config["path"] = path;
			const bool sheet = source.contains("size");
			if (sheet)
			{
				(void)grid(vectorMember(source, "size"), false);
				config["size"] = source.value().at("size");
			}
			addLoadedRole(builder, assets, role, entry, sheet ? "SpriteSheet" : "Texture", path, config);
		}

		void addAssetRole(Material::Builder &builder, Asset::Manager &assets, const std::string &role,
						  const Reader &entry, const Reader &source)
		{
			const Reader asset = source.child("asset");
			asset.forbidUnknown({"type", "id", "config"});
			const std::string type = nonEmptyString(asset, "type");
			const std::string id = nonEmptyString(asset, "id");
			if (type != "Texture" && type != "SpriteSheet")
			{
				throw Exception(asset.pathFor("type") + ": expected Texture or SpriteSheet");
			}
			const JSON::Value config = asset.contains("config") ? asset.value().at("config") : JSON::Value::object();
			validateConfiguration(type, config, asset.pathFor("config"));
			addLoadedRole(builder, assets, role, entry, type, id, config);
		}

		void addRole(Material::Builder &builder, Asset::Manager &assets, const std::string &role, const Reader &entry)
		{
			entry.forbidUnknown({"Texture", "UV"});
			const Reader source = entry.child("Texture");
			source.forbidUnknown({"path", "size", "asset"});
			const bool hasPath = source.contains("path");
			if (hasPath == source.contains("asset"))
			{
				throw Exception(source.path() + ": exactly one of path or asset is required");
			}
			if (hasPath)
			{
				addPathRole(builder, assets, role, entry, source);
				return;
			}
			if (source.contains("size"))
			{
				throw Exception(source.pathFor("size") + ": size is not allowed with asset");
			}
			addAssetRole(builder, assets, role, entry, source);
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
