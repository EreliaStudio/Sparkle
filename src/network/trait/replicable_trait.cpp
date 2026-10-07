#include "network/trait/replicable_trait.hpp"

namespace spk::Network
{
	void ReplicableTrait::readNetworkState(const spk::Message::Reader &reader)
	{
		OperationGuard guard(_reading);
		_readNetworkState(reader);
		if (reader.readOffset() != reader.size())
		{
			throw spk::Exception("Trailing object state bytes");
		}
	}
}
