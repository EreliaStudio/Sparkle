#include "game_engine/component/spk_game_component.hpp"
#include "game_engine/spk_game_object.hpp"
#include "application/spk_application.hpp"

namespace spk
{
	GameComponent::GameComponent(const std::string& p_name) :
	#ifndef NDEBUG
		_timeMetric(spk::Application::activeApplication()->profiler().metric<TimeMetric>("Component : " + fullName())),
	#endif
		_owner(_creatingObject),
		_name(p_name)
	{
		activate();
	}

	void GameComponent::update()
	{
	#ifndef NDEBUG
		_timeMetric.start();
	#endif
		_onUpdate();
	#ifndef NDEBUG
		_timeMetric.stop();
	#endif
	}
	
	void GameComponent::render()
	{
		_onRender();
	}

	GameObject* GameComponent::owner()
	{
		return (_owner);
	}

	const GameObject* GameComponent::owner() const
	{
		return (_owner);
	}

	std::string GameComponent::fullName() const
	{
		return (owner()->fullName() + "::" + name());
	}

	const std::string& GameComponent::name() const
	{
		return (_name);
	}
}