#pragma once

#include <string>

#include "asset/factory.hpp"
#include "asset/manager.hpp"
#include "container/json/object.hpp"
#include "graphics/material.hpp"

namespace spk
{
	template <>
	struct AssetFactory<Texture>
	{
		[[nodiscard]] static Texture create(Assets &assets, const std::string &id, const JSON::Value &config);
	};

	template <>
	struct AssetFactory<SpriteSheet>
	{
		[[nodiscard]] static SpriteSheet create(Assets &assets, const std::string &id, const JSON::Value &config);
	};

	template <>
	struct AssetFactory<Material>
	{
		[[nodiscard]] static Material create(Assets &assets, const std::string &id, const JSON::Value &config);
	};
}
