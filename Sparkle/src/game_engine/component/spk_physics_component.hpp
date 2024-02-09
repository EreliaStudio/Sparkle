#pragma once

#include "game_engine/component/spk_game_component.hpp"
#include "math/spk_vector3.hpp"

namespace spk
{
	class Physics : public GameComponent
	{
	private:
		bool _isKinematic;
		float _mass;
		spk::Vector3 _velocity;

		void _onRender();
		void _onUpdate();

	public:
		Physics(const std::string& p_name);

		bool isKinematic();
		void setKinematicState(bool p_state);

		void setMass(const float& p_mass);
		void applyForce(const spk::Vector3& p_force);
		void applyAcceleration(const spk::Vector3& p_acceleration);

		const float& mass() const;
	};
}