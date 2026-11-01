#include "graphics/sprite_sheet.hpp"

#include <exception>
#include <filesystem>

#include "container/json/reader.hpp"
#include "exception.hpp"

namespace spk::Asset
{
	namespace
	{
		[[nodiscard]] spk::Vector2UInt grid(const JSON::Reader &reader)
		{
			spk::Vector2UInt value;
			try
			{
				value = reader.value().as<spk::Vector2UInt>();
			} catch (...)
			{
				throw spk::Exception(reader.path() + ": expected two unsigned coordinates", std::current_exception());
			}
			if (value.x == 0 || value.y == 0)
			{
				throw spk::Exception(reader.path() + ": expected positive grid dimensions");
			}
			return value;
		}
	}

	spk::SpriteSheet Factory<spk::SpriteSheet>::create(Manager &, const std::string &, const JSON::Value &config)
	{
		JSON::Reader reader(config, {}, "SpriteSheet.config");
		reader.forbidUnknown({"path", "size"});
		const std::string path = reader.require<std::string>("path");
		if (path.empty())
		{
			throw spk::Exception(reader.pathFor("path") + ": expected non-empty path");
		}
		if (!reader.contains("size"))
		{
			throw spk::Exception(reader.pathFor("size") + ": missing sprite grid");
		}
		const JSON::Reader sizeReader(reader.value().at("size"), {}, reader.pathFor("size"));
		const spk::Vector2UInt size = grid(sizeReader);
		try
		{
			return spk::SpriteSheet::open(std::filesystem::path(path), size);
		} catch (...)
		{
			throw spk::Exception("SpriteSheet asset: failed to load image '" + path + "'", std::current_exception());
		}
	}
}
