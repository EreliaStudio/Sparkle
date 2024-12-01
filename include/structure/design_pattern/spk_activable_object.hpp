#pragma once

#include "structure/design_pattern/spk_stateful_object.hpp"

namespace spk
{
	class ActivableObject : public StatefulObject<bool>
	{
	public:
		using Contract = StatefulObject::Contract;

	public:
		ActivableObject();
		virtual ~ActivableObject();

		void activate();
		void deactivate();
		bool isActive() const;

		Contract addActivationCallback(const std::function<void()>& p_callback);
		Contract addDeactivationCallback(const std::function<void()>& p_callback);

	private:
		using StatefulObject<bool>::setState;
		using StatefulObject<bool>::addCallback;
		using StatefulObject<bool>::state;
	};
}