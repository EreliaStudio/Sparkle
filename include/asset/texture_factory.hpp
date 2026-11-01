#pragma once

#include <string>

#include "asset/manager.hpp"
#include "graphics/opengl/texture.hpp"

namespace spk::Asset
{
	template <>
	struct Factory<Texture>
	{
		[[nodiscard]] static Texture create(Manager &assets, const std::string &id, const JSON::Value &config);
	};
}
