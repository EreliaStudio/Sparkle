#pragma once
#include "network/replication/types.hpp"
#include <map>
#include <cstdint>

namespace spk::Network
{
	// Remembers the last accepted request ID independently of pending acquisitions.
	class IncomingRequestHistoryTrait
	{
		std::map<PeerID, spk::Message::RequestID> _lastRequests;

	protected:
		[[nodiscard]] bool _isNewRequest(PeerID peer, spk::Message::RequestID id) const;
		void _recordRequest(PeerID peer, spk::Message::RequestID id);
		void _clearRequestHistory(PeerID peer);

	public:
		virtual ~IncomingRequestHistoryTrait() = default;
	};
}
