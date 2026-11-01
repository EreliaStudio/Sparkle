#include "graphics/opengl/texture.hpp"

#include <exception>
#include <filesystem>

#include "container/json/reader.hpp"
#include "exception.hpp"
#include "graphics/image.hpp"

namespace spk::Asset
{
	spk::Texture Factory<spk::Texture>::create(Manager &, const std::string &, const JSON::Value &config)
	{
		JSON::Reader reader(config, {}, "Texture.config");
		reader.forbidUnknown({"path"});
		const std::string path = reader.require<std::string>("path");
		if (path.empty())
		{
			throw spk::Exception(reader.pathFor("path") + ": expected non-empty path");
		}
		try
		{
			return spk::Image::open(std::filesystem::path(path));
		} catch (...)
		{
			throw spk::Exception("Texture asset: failed to load image '" + path + "'", std::current_exception());
		}
	}
}
