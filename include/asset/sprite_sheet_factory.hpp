#pragma once

#include <string>

#include "asset/manager.hpp"
#include "graphics/sprite_sheet.hpp"

namespace spk::Asset
{
	template <>
	struct Factory<SpriteSheet>
	{
		[[nodiscard]] static SpriteSheet create(Manager &assets, const std::string &id, const JSON::Value &config);
	};
}
