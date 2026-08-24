#pragma once

#include "type/activation_status.hpp"
#include "design_pattern/trait/statefull_trait.hpp"

namespace spk
{
	class ActivableTrait : private StatefullTrait<ActivationStatus>
	{
	private:
		using Base = StatefullTrait<ActivationStatus>;

	public:
		using ActivationCallback = Base::OnStateEditionCallback;
		using DeactivationCallback = Base::OnStateEditionCallback;

		using ActivationContract = Base::OnStateEditionContract;
		using DeactivationContract = Base::OnStateEditionContract;

		ActivableTrait();
		explicit ActivableTrait(ActivationStatus status);

		void activate();
		void deactivate();
		bool isActive() const;
		void toggle();

		ActivationContract subscribeToActivation(ActivationCallback callback);
		DeactivationContract subscribeToDeactivation(DeactivationCallback callback);
	};
}
