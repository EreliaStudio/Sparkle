#pragma once

#include "structure/graphics/spk_color.hpp"
#include "structure/math/spk_vector3.hpp"

namespace spk
{
	struct DirectionalLight
	{
		spk::Vector3 direction = spk::Vector3(0, -1, 0);
		spk::Color color = spk::Color(255, 255, 255);
		float ambiantPower = 0.1f;
	};
}