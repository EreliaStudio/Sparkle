#pragma once

#include <string>

#include "asset/manager.hpp"
#include "graphics/material.hpp"

namespace spk::Asset
{
	template <>
	struct Factory<Material>
	{
		[[nodiscard]] static Material create(Manager &assets, const std::string &id, const JSON::Value &config);
	};
}
