#pragma once

#include "network/replication/interest.hpp"
#include "network/types.hpp"

namespace spk::Network
{
	class ServerReplicatedComponent;
	class InterestEvaluator
	{
	public:
		virtual ~InterestEvaluator() = default;
		[[nodiscard]] virtual bool matches(const Interest &interest, const ServerReplicatedComponent &component, spk::ConnectionID client) const = 0;
	};
}
