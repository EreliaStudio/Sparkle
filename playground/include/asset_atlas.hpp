#pragma once

#include <sparkle.hpp>

namespace taag
{
	class AssetAtlas : public spk::Singleton<AssetAtlas>, public spk::AssetAtlas
	{
		friend class spk::Singleton<AssetAtlas>;

	private:
		AssetAtlas()
		{
			load("playground/resources/assets_configuration.json");
		}
	};
}