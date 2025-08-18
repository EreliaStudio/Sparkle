#include "structure/design_pattern/spk_activable_object.hpp"

namespace spk
{
	ActivableObject::ActivableObject() :
		StatefulObject<bool>(false)
	{
	}
	
	void ActivableObject::activate()
	{
		StatefulObject<bool>::setState(true);
	}

	void ActivableObject::deactivate()
	{
		StatefulObject<bool>::setState(false);
	}

	bool ActivableObject::isActive() const
	{
		return state();
	}

	ActivableObject::Contract ActivableObject::addActivationCallback(const Job &p_callback)
	{
		return (std::move(addCallback(true, p_callback)));
	}

	ActivableObject::Contract ActivableObject::addDeactivationCallback(const Job &p_callback)
	{
		return (std::move(addCallback(false, p_callback)));
	}
}