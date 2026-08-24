#include "design_pattern/trait/activable_trait.hpp"

#include <utility>

namespace spk
{
	ActivableTrait::ActivableTrait() :
		ActivableTrait(ActivationStatus::Deactivated)
	{
	}

	ActivableTrait::ActivableTrait(ActivationStatus status)
	{
		Base::setState(status);
	}

	void ActivableTrait::activate()
	{
		Base::setState(ActivationStatus::Activated);
	}

	void ActivableTrait::deactivate()
	{
		Base::setState(ActivationStatus::Deactivated);
	}

	bool ActivableTrait::isActive() const
	{
		return Base::state() == ActivationStatus::Activated;
	}

	void ActivableTrait::toggle()
	{
		isActive() ? deactivate() : activate();
	}

	ActivableTrait::ActivationContract ActivableTrait::subscribeToActivation(ActivationCallback callback)
	{
		return Base::subscribeToStateEnter(ActivationStatus::Activated, std::move(callback));
	}

	ActivableTrait::DeactivationContract ActivableTrait::subscribeToDeactivation(DeactivationCallback callback)
	{
		return Base::subscribeToStateEnter(ActivationStatus::Deactivated, std::move(callback));
	}
}
