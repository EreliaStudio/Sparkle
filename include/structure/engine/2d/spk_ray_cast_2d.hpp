#pragma once

#include <span>
#include <string>
#include <vector>

#include "structure/engine/2d/spk_entity_2d.hpp"
#include "structure/math/spk_vector2.hpp"
#include "structure/system/spk_boolean_enum.hpp"

namespace spk
{
	class RayCast2D
	{
	public:
		struct Hit
		{
			spk::Vector2 position;
			spk::SafePointer<spk::Entity2D> entity;
		};

		static Hit launch(
			const spk::SafePointer<spk::Entity2D> &p_cameraEntity,
			const spk::Vector2 &p_screenPosition,
			float p_maxDistance,
			std::span<const std::wstring> p_tags = {},
			spk::BinaryOperator p_binaryOperator = spk::BinaryOperator::AND);
		static std::vector<Hit> launchAll(
			const spk::SafePointer<spk::Entity2D> &p_cameraEntity,
			const spk::Vector2 &p_screenPosition,
			float p_maxDistance,
			std::span<const std::wstring> p_tags = {},
			spk::BinaryOperator p_binaryOperator = spk::BinaryOperator::AND);

		static spk::Vector2 screenPointToWorld(
			const spk::SafePointer<spk::Entity2D> &p_cameraEntity, const spk::Vector2 &p_screenPosition, float p_layer);
	};
}
