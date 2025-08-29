#pragma once

#include <vector>

#include "structure/engine/spk_entity.hpp"
#include "structure/math/spk_vector2.hpp"

namespace spk
{
	class RayCast2D
	{
	public:
		struct Hit
		{
			spk::Vector2 position;
			spk::SafePointer<spk::Entity> entity;
		};

		static Hit launch(const spk::SafePointer<spk::Entity> &p_cameraEntity, const spk::Vector2 &p_screenPosition, float p_maxDistance);
		static std::vector<Hit> launchAll(
			const spk::SafePointer<spk::Entity> &p_cameraEntity, const spk::Vector2 &p_screenPosition, float p_maxDistance);

		static spk::Vector2 screenPointToWorld(
			const spk::SafePointer<spk::Entity> &p_cameraEntity, const spk::Vector2 &p_screenPosition, float p_layer);
	};
}
