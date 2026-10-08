#pragma once
#include "network/replication/update.hpp"
#include <deque>
#include <map>
namespace spk::Network
{
	// Retains coalesced updates until the owner confirms successful delivery.
	class PublicationQueueTrait
	{
	protected:
		struct Pending
		{
			Update<spk::Message> update;
			spk::Message::RequestID requestID = 0;
		};

	private:
		struct Queue
		{
			std::map<ObjectID, Pending> pending;
			std::deque<ObjectID> order;
		};
		std::map<PeerID, Queue> _queues;
		std::size_t _maximumPending;

	protected:
		void _openQueue(PeerID peer);
		void _closeQueue(PeerID peer);
		void _room(PeerID peer, ObjectID id) const;
		void _queue(PeerID peer, Update<spk::Message> update, spk::Message::RequestID requestID = 0);
		[[nodiscard]] const Pending *_front(PeerID peer) const;
		void _pop(PeerID peer);

	public:
		explicit PublicationQueueTrait(std::size_t maximumPending = 16384);
		PublicationQueueTrait(const PublicationQueueTrait &) = delete;
		PublicationQueueTrait &operator=(const PublicationQueueTrait &) = delete;
		virtual ~PublicationQueueTrait() = default;
	};
}
