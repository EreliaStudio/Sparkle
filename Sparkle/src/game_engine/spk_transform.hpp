#pragma once

#include "math/spk_vector3.hpp"
#include "game_engine/component/spk_game_component.hpp"
#include "design_pattern/spk_observable_value.hpp"

namespace spk
{
	class Transform
	{
	private:
		using ObservableVector3 = spk::ObservableValue<spk::Vector3>;

	public:
		using Contract = spk::ObservableValue<spk::Vector3>::Contract;
		ObservableVector3 translation; 
		ObservableVector3 scale;
		ObservableVector3 rotation;
		std::unique_ptr<Contract> rotationContract;
		
		spk::Vector3 _forward;
		spk::Vector3 _right;
		spk::Vector3 _up;

	private:
		void _computeDirections();

	public:
		Transform();
		Transform(const Transform& p_other);
		Transform& operator = (const Transform& p_other);

		void lookAt(const spk::Vector3& p_target);

		const spk::Vector3& forward() const;
		const spk::Vector3& right() const;

		const spk::Vector3& up() const;
	};
}