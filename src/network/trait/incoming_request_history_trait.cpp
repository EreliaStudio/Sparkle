#include "network/trait/incoming_request_history_trait.hpp"
#include "exception.hpp"

namespace spk::Network
{
	bool IncomingRequestHistoryTrait::_isNewRequest(PeerID peer, spk::Message::RequestID id) const
	{
		const auto found = _lastRequests.find(peer);
		return found == _lastRequests.end() || id > found->second;
	}
	void IncomingRequestHistoryTrait::_recordRequest(PeerID peer, spk::Message::RequestID id)
	{
		auto [entry, inserted] = _lastRequests.try_emplace(peer, id);
		if (!inserted)
		{
			if (id <= entry->second)
			{
				throw spk::Exception("Request identity must increase");
			}
			entry->second = id;
		}
	}
	void IncomingRequestHistoryTrait::_clearRequestHistory(PeerID peer)
	{
		_lastRequests.erase(peer);
	}
}
