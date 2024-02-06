#include "game_engine/component/spk_game_component.hpp"
#include "game_engine/spk_game_object.hpp"

namespace spk
{
	GameComponent::GameComponent(const std::string& p_name) :
		_owner(_creatingObject),
		_name(p_name)
	{
		activate();
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