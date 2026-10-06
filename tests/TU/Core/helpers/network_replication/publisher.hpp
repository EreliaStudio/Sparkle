#pragma once
#include "state.hpp"
#include <network/replication/publisher.hpp>
namespace ReplicationTest
{
	inline spk::Network::Publisher<State> publisher()
	{
		return spk::Network::Publisher<State>({.interval = std::chrono::milliseconds(0)});
	}
} // namespace ReplicationTest
