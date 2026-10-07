#pragma once
#include "exception.hpp"
#include "network/message.hpp"
#include "network/replication/update.hpp"
#include <algorithm>
#include <deque>
#include <map>
#include <set>
namespace spk::Network
{
	// Protected operations are coordinated by an owner-thread publication behavior.
	class PublicationQueueTrait
	{
	public:
		struct DispatchResult
		{
			std::size_t sent = 0, blocked = 0, errors = 0;
		};

	private:
		struct Pending
		{
			Update<spk::Message> update;
			spk::Message::RequestID requestID = 0;
		};
		struct Queue
		{
			std::map<ObjectID, Pending> pending;
			std::deque<ObjectID> order;
		};
		std::map<PeerID, Queue> _queues;
		std::deque<PeerID> _roundRobin;
		std::size_t _maximumPending;
		Clock::duration _interval;
		Clock::time_point _nextPublication = Clock::time_point::min();
		bool _dispatchOne(PeerID peer, DispatchResult &result);

	protected:
		virtual void _capturePublicationChanges();
		[[nodiscard]] virtual bool _sendUpdate(PeerID, const Update<spk::Message> &, spk::Message::RequestID) = 0;
		void _openQueue(PeerID peer);
		void _closeQueue(PeerID peer);
		void _room(PeerID peer, ObjectID id) const;
		void _queue(PeerID peer, Update<spk::Message> update, spk::Message::RequestID requestID = 0);
		DispatchResult _dispatchPublication(Clock::time_point now, std::size_t maximumAttempts);

	public:
		explicit PublicationQueueTrait(std::size_t maximumPending = 16384, Clock::duration interval = std::chrono::milliseconds(50));
		PublicationQueueTrait(const PublicationQueueTrait &) = delete;
		PublicationQueueTrait &operator=(const PublicationQueueTrait &) = delete;
		virtual ~PublicationQueueTrait() = default;
	};
}
