#pragma once

#include <string>

#include "asset/factory.hpp"
#include "asset/manager.hpp"
#include "container/json/object.hpp"
#include "graphics/material.hpp"

namespace spk::Asset
{
	template <>
	struct Factory<Texture>
	{
		[[nodiscard]] static Texture create(Manager &assets, const std::string &id, const JSON::Value &config);
	};

	template <>
	struct Factory<SpriteSheet>
	{
		[[nodiscard]] static SpriteSheet create(Manager &assets, const std::string &id, const JSON::Value &config);
	};

	template <>
	struct Factory<Material>
	{
		[[nodiscard]] static Material create(Manager &assets, const std::string &id, const JSON::Value &config);
	};
}
