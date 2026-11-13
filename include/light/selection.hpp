#pragma once

#include <cstdint>
#include <vector>

#include "graphics/color.hpp"
#include "math/vector3.hpp"

namespace spk::Light
{
	// CPU values only: a published Selection never reads a mutable Component or Entity.
	struct Selection
	{
		enum class Type
		{
			Point,
			Spot,
			Directional
		};

		struct Entry
		{
			// Registration order is the identity of this registration in its Engine.
			std::uint64_t registrationOrder = 0;
			std::uint64_t admission = 0;
			Type type = Type::Point;
			RGB color{1.0f, 1.0f, 1.0f};
			float intensity = 0.0f;
			float priorityWeight = 0.0f;
			float range = 0.0f;
			float angle = 0.0f;
			Vector3 position{};
			Vector3 direction{};
			float shadowDistance = 0.0f;
			bool castsShadows = false;
			std::uint32_t shadowResolution = 1024;
			float score = 0.0f;
		};

		std::uint64_t generation = 0;
		std::uint64_t captureOrdinal = 0;
		std::vector<Entry> local;
		std::vector<Entry> directional;
	};
}
