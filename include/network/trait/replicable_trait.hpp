#pragma once

#include "network/replication/operation_guard.hpp"

namespace spk::Network
{
	template <typename State>
	class ReplicableTrait
	{
		bool _applying = false;

	protected:
		// Validate before changing the object; a throwing hook must leave it
		// unchanged so the same update can be retried.
		virtual void _applyNetworkState(const State &state) = 0;

	public:
		virtual ~ReplicableTrait() = default;

		virtual void applyNetworkState(const State &state) final
		{
			OperationGuard guard(_applying);
			_applyNetworkState(state);
		}
	};
}
