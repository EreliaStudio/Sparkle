#include "network/trait/publishable_trait.hpp"

namespace spk::Network
{
	void PublishableTrait::writeNetworkState(spk::Message::Writer &writer) const
	{
		_writeNetworkState(writer);
	}
	spk::Message PublishableTrait::captureNetworkState() const
	{
		spk::Message::Writer writer;
		writeNetworkState(writer);
		return std::move(writer).build();
	}
}
