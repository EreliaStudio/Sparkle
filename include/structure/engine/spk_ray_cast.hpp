#pragma once

#include <span>
#include <string>
#include <vector>

#include "structure/engine/spk_entity.hpp"
#include "structure/engine/spk_game_engine.hpp"
#include "structure/math/spk_vector3.hpp"
#include "structure/system/spk_boolean_enum.hpp"

namespace spk
{
	class RayCast
	{
	public:
		struct Hit
		{
			spk::Vector3 position;
			spk::SafePointer<spk::Entity> entity;
		};

		static Hit launch(
			const spk::SafePointer<spk::GameEngine> &p_engine,
			const spk::Vector3 &p_eye,
			const spk::Vector3 &p_direction,
			float p_maxDistance,
			std::span<const std::wstring> p_tags = {},
			spk::BinaryOperator p_binaryOperator = spk::BinaryOperator::AND);
		static std::vector<Hit> launchAll(
			const spk::SafePointer<spk::GameEngine> &p_engine,
			const spk::Vector3 &p_eye,
			const spk::Vector3 &p_direction,
			float p_maxDistance,
			std::span<const std::wstring> p_tags = {},
			spk::BinaryOperator p_binaryOperator = spk::BinaryOperator::AND);

		static Hit launch(
			const spk::SafePointer<spk::Entity> &p_entity,
			const spk::Vector3 &p_direction,
			float p_maxDistance,
			std::span<const std::wstring> p_tags = {},
			spk::BinaryOperator p_binaryOperator = spk::BinaryOperator::AND);
		static std::vector<Hit> launchAll(
			const spk::SafePointer<spk::Entity> &p_entity,
			const spk::Vector3 &p_direction,
			float p_maxDistance,
			std::span<const std::wstring> p_tags = {},
			spk::BinaryOperator p_binaryOperator = spk::BinaryOperator::AND);
	};
}
