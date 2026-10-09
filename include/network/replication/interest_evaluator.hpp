#pragma once

#include "network/replication/interest.hpp"

namespace spk
{
	using ConnectionID = std::uint64_t;
}

namespace spk::Network
{
	class ServerReplicatedComponent;
	class InterestEvaluator
	{
	public:
		virtual ~InterestEvaluator() = default;
		[[nodiscard]] virtual bool matches(const Interest &interest,
			const ServerReplicatedComponent &component, spk::ConnectionID client) const = 0;
	};
}
