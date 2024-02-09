#pragma once

#include "game_engine/component/spk_game_component.hpp"
#include "math/spk_vector3.hpp"

namespace spk
{
	class Physics : public GameComponent
	{
	private:
		void _onRender();
		void _onUpdate();

	public:
		Physics(const std::string& p_name);

		void applyForce(const spk::Vector3& p_force);
	};
}