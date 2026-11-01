#include "graphics/material.hpp"

#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <string>
#include <utility>

#include "asset/manager.hpp"
#include "container/json/reader.hpp"
#include "exception.hpp"

namespace spk
{
	namespace
	{
		using Reader = JSON::Reader;

		struct ResolvedTexture
		{
			const Texture *texture;
			const SpriteSheet *sheet = nullptr;
		};

		[[nodiscard]] std::string nonEmptyString(const Reader &reader, const std::string &key)
		{
			const std::string value = reader.require<std::string>(key);
			if (value.empty())
			{
				throw Exception(reader.pathFor(key) + ": expected non-empty string");
			}
			return value;
		}

		[[nodiscard]] Vector2UInt grid(const Reader &reader, bool allowZero)
		{
			reader.forbidUnknown({"x", "y"});
			const unsigned int x = reader.require<unsigned int>("x");
			const unsigned int y = reader.require<unsigned int>("y");
			if (!allowZero && (x == 0 || y == 0))
			{
				throw Exception(reader.path() + ": grid dimensions must be positive");
			}
			return {x, y};
		}

		[[nodiscard]] Vector2 normalizedPoint(const Reader &reader)
		{
			reader.forbidUnknown({"x", "y"});
			const auto component = [&](const std::string &key) {
				const JSON::Value &value = reader.value().at(key);
				if (!value.isNumber())
				{
					throw Exception(reader.pathFor(key) + ": expected number");
				}
				const double number = reader.require<double>(key);
				if (!std::isfinite(number) || number < 0.0 || number > 1.0)
				{
					throw Exception(reader.pathFor(key) + ": expected finite number in [0, 1]");
				}
				return static_cast<float>(number);
			};
			if (!reader.contains("x") || !reader.contains("y"))
			{
				throw Exception(reader.path() + ": expected both x and y");
			}
			return {component("x"), component("y")};
		}

		void validateConfiguration(const std::string &type, const JSON::Value &config, const std::string &path)
		{
			Reader reader(config, {}, path);
			if (type == "Texture")
			{
				reader.forbidUnknown({});
				return;
			}
			reader.forbidUnknown({"size"});
			(void)grid(reader.child("size"), false);
		}

		[[nodiscard]] ResolvedTexture resolveTexture(asset::Manager &assets, const Reader &description)
		{
			description.forbidUnknown({"path", "size", "asset"});
			const bool path = description.contains("path");
			if (path == description.contains("asset"))
			{
				throw Exception(description.path() + ": exactly one of path or asset is required");
			}

			std::string type;
			std::string id;
			JSON::Value config = JSON::Value::object();

			if (path)
			{
				id = nonEmptyString(description, "path");
				type = description.contains("size") ? "SpriteSheet" : "Texture";
				if (type == "SpriteSheet")
				{
					(void)grid(description.child("size"), false);
					config["size"] = description.value().at("size");
				}
			}
			else
			{
				const Reader source = description.child("asset");
				source.forbidUnknown({"type", "id", "config"});
				type = nonEmptyString(source, "type");
				id = nonEmptyString(source, "id");
				if (type != "Texture" && type != "SpriteSheet")
				{
					throw Exception(source.pathFor("type") + ": expected Texture or SpriteSheet");
				}
				if (source.contains("config"))
				{
					config = source.value().at("config");
				}
				validateConfiguration(type, config, source.pathFor("config"));
				if (description.contains("size"))
				{
					throw Exception(description.pathFor("size") + ": size is not allowed with asset");
				}
			}

			try
			{
				if (type == "SpriteSheet")
				{
					const SpriteSheet &sheet = assets.load<SpriteSheet>(id, config);
					return {&sheet, &sheet};
				}
				return {&assets.load<Texture>(id, config), nullptr};
			} catch (...)
			{
				throw Exception(description.path() + ": failed to load " + type + " '" + id + "'", std::current_exception());
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
				const Vector2UInt coord = grid(uv.child("sprite"), true);
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
			const Vector2 anchor = normalizedPoint(uv.child("anchor"));
			const Vector2 size = normalizedPoint(uv.child("size"));
			if (size.x <= 0.0f || size.y <= 0.0f ||
				double(anchor.x) + size.x > 1.0 || double(anchor.y) + size.y > 1.0)
			{
				throw Exception(uv.path() + ": invalid normalized rectangle");
			}
			return {anchor, size};
		}
	}

	Material::Binding Material::Builder::_readRole(const JSON::Reader &reader) const
	{
		reader.forbidUnknown({"Texture", "UV"});
		const ResolvedTexture resolved = resolveTexture(*_assets, reader.child("Texture"));
		return Binding{resolved.texture->handle(), readSection(reader, resolved.sheet)};
	}

	void Material::Builder::load(const JSON::Value &description)
	{
		if (_assets == nullptr)
		{
			throw Exception("Material::Builder.load: bind an asset manager first");
		}
		const Reader root(description, {}, "material");
		root.forbidUnknown({"textures"});
		const Reader textures = root.child("textures");
		Textures next = _textures;
		for (const auto &[role, value] : textures.value().asObject())
		{
			if (role.empty() || role.find_first_not_of(" \t\n\r\f\v") == std::string::npos)
			{
				throw Exception(textures.pathFor(role) + ": invalid role name");
			}
			if (next.contains(role))
			{
				throw Exception(textures.pathFor(role) + ": duplicate role");
			}
			const Reader entry(value, {}, textures.pathFor(role));
			next.emplace(role, _readRole(entry));
		}
		_textures.swap(next);
	}
}

namespace spk::asset
{
	spk::Texture Factory<spk::Texture>::create(Manager &, const std::string &id, const JSON::Value &config)
	{
		JSON::Reader reader(config, {}, "Texture.config");
		reader.forbidUnknown({});
		if (id.empty())
		{
			throw spk::Exception("Texture asset: path cannot be empty");
		}
		try
		{
			return spk::Image::open(std::filesystem::path(id));
		} catch (...)
		{
			throw spk::Exception("Texture asset: failed to load image '" + id + "'", std::current_exception());
		}
	}

	spk::SpriteSheet Factory<spk::SpriteSheet>::create(Manager &, const std::string &id, const JSON::Value &config)
	{
		JSON::Reader reader(config, {}, "SpriteSheet.config");
		reader.forbidUnknown({"size"});
		const spk::Vector2UInt size = spk::grid(reader.child("size"), false);
		if (id.empty())
		{
			throw spk::Exception("SpriteSheet asset: path cannot be empty");
		}
		try
		{
			return spk::SpriteSheet::open(std::filesystem::path(id), size);
		} catch (...)
		{
			throw spk::Exception("SpriteSheet asset: failed to load image '" + id + "'", std::current_exception());
		}
	}

	spk::Material Factory<spk::Material>::create(Manager &assets, const std::string &, const JSON::Value &config)
	{
		spk::Material::Builder builder;
		builder.bind(&assets);
		builder.load(config);
		return builder.build();
	}
}
