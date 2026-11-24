#pragma once

#include "math/vector2.hpp"
#include "world/biome.hpp"

namespace spk::World
{
	class SurfaceBiome : public Biome
	{
	public:
		struct Influence
		{
			const SurfaceBiome *biome = nullptr;
			float weight = 0.0f;
		};

		[[nodiscard]] virtual float groundHeight(const spk::Vector2 &worldPosition) const = 0;
	};
}
