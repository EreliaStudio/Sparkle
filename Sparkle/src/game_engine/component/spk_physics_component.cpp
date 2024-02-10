#include "game_engine/component/spk_physics_component.hpp"
#include "game_engine/spk_game_object.hpp"
#include "application/spk_application.hpp"

namespace spk
{
	void Physics::_onRender()
	{

	}

	void Physics::_onUpdate()
	{
		owner()->transform().translation += nextVelocityTick();
	}
	
	Physics::Physics(const std::string& p_name) :
		GameComponent(p_name),
		_velocity(0, 0, 0),
		_mass(1.0f),
		_isKinematic(false)
	{

	}

	bool Physics::isKinematic()
	{
		return (_isKinematic);
	}
	
	void Physics::setKinematicState(bool p_state)
	{
		_isKinematic = p_state;
	}

	void Physics::setMass(const float& p_mass)
	{
		_mass = p_mass;
	}

	void Physics::setVelocity(const spk::Vector3& p_velocity)
	{
		_velocity = p_velocity;
	}

	void Physics::applyForce(const spk::Vector3& p_force)
	{
		const float SecondToMillisecondRatio = 1000.0f;
		_velocity += (p_force * (1.0f / _mass)) / SecondToMillisecondRatio;
	}

	void Physics::applyAcceleration(const spk::Vector3& p_acceleration)
	{
		const float SecondToMillisecondRatio = 1000.0f;
		_velocity += p_acceleration / SecondToMillisecondRatio;
	}

	const float& Physics::mass() const
	{
		return (_mass);
	}

	const spk::Vector3& Physics::velocity() const
	{
		return (_velocity);
	}

	spk::Vector3 Physics::nextVelocityTick() const
	{
		return (_velocity * static_cast<float>(spk::Application::activeApplication()->timeManager().deltaTime()));
	}
}