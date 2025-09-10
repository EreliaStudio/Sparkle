#pragma once

#include "structure/design_pattern/spk_stateful_object.hpp"

namespace spk
{
	class ActivableObject : public StatefulObject<bool>
	{
	public:
		using ActivationContract = StatefulObject::Contract;
		using DeactivationContract = StatefulObject::Contract;
		using Job = StatefulObject::Job;

	public:
		ActivableObject();
		~ActivableObject() override;

		void activate();
		void deactivate();
		bool isActive() const;

		ActivationContract addActivationCallback(const Job &p_callback);
		DeactivationContract addDeactivationCallback(const Job &p_callback);

	private:
		using StatefulObject<bool>::setState;
		using StatefulObject<bool>::addCallback;
		using StatefulObject<bool>::state;
	};
}