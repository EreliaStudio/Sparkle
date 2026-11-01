#include "graphics/opengl/texture.hpp"

#include <exception>
#include <filesystem>

#include "container/json/reader.hpp"
#include "exception.hpp"
#include "graphics/image.hpp"

namespace spk::Asset
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
}
