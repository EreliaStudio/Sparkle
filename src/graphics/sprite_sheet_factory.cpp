#include "graphics/sprite_sheet.hpp"

#include <exception>
#include <filesystem>

#include "container/json/reader.hpp"
#include "exception.hpp"

namespace spk::Asset
{
	spk::SpriteSheet Factory<spk::SpriteSheet>::create(Manager &, const std::string &, const JSON::Value &config)
	{
		JSON::Reader reader(config, {}, "SpriteSheet.config");
		reader.forbidUnknown({"path", "size"});
		const std::string path = reader.require<std::string>(
			"path",
			[](const std::string &value) {
				return !value.empty();
			},
			"expected non-empty path");
		const spk::Vector2UInt size = reader.require<spk::Vector2UInt>(
			"size",
			[](const spk::Vector2UInt &value) {
				return value.x > 0 && value.y > 0;
			},
			"expected positive grid dimensions");
		try
		{
			return spk::SpriteSheet::open(std::filesystem::path(path), size);
		} catch (...)
		{
			throw spk::Exception("SpriteSheet asset: failed to load image '" + path + "'", std::current_exception());
		}
	}
}
