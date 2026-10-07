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
	template <typename State>
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
			Update<State> update;
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
		bool _dispatchOne(PeerID peer, DispatchResult &result)
		{
			auto &queue = _queues.at(peer);
			if (queue.order.empty())
			{
				return false;
			}
			const auto &pending = queue.pending.at(queue.order.front());
			try
			{
				if (_sendUpdate(peer, pending.update, pending.requestID))
				{
					queue.pending.erase(queue.order.front());
					queue.order.pop_front();
					++result.sent;
					return true;
				}
			} catch (...)
			{
				++result.errors;
			}
			++result.blocked;
			return false;
		}

	protected:
		virtual void _capturePublicationChanges()
		{
		}
		[[nodiscard]] virtual bool _sendUpdate(PeerID, const Update<State> &, spk::Message::RequestID) = 0;
		void _openQueue(PeerID peer)
		{
			if (_queues.contains(peer))
			{
				throw spk::Exception("Duplicate publication queue");
			}
			auto [entry, inserted] = _queues.try_emplace(peer);
			try
			{
				_roundRobin.push_back(peer);
			} catch (...)
			{
				_queues.erase(entry);
				throw;
			}
		}
		void _closeQueue(PeerID peer)
		{
			_queues.erase(peer);
			std::erase(_roundRobin, peer);
		}
		void _room(PeerID peer, ObjectID id) const
		{
			auto found = _queues.find(peer);
			if (found == _queues.end())
			{
				throw spk::Exception("Unknown publication queue");
			}
			if (!found->second.pending.contains(id) && found->second.pending.size() >= _maximumPending)
			{
				throw spk::Exception("Replication queue full; drain or close peer");
			}
		}
		void _queue(PeerID peer, Update<State> update, spk::Message::RequestID requestID = 0)
		{
			_room(peer, update.object);
			auto &queue = _queues.at(peer);
			auto [found, inserted] = queue.pending.try_emplace(update.object);
			if (inserted)
			{
				try
				{
					queue.order.push_back(update.object);
				} catch (...)
				{
					queue.pending.erase(found);
					throw;
				}
			}
			found->second.update = std::move(update);
			if (found->second.update.edit != Edit::Set)
			{
				found->second.requestID = 0;
			}
			else if (requestID != 0)
			{
				found->second.requestID = requestID;
			}
		}
		DispatchResult _dispatchPublication(Clock::time_point now, std::size_t maximumAttempts)
		{
			DispatchResult result;
			if (now < _nextPublication || maximumAttempts == 0 || _queues.empty())
			{
				return result;
			}
			_capturePublicationChanges();
			_nextPublication = now + _interval;
			std::set<PeerID> skipped;
			for (std::size_t count = 0; count < maximumAttempts && skipped.size() < _queues.size(); ++count)
			{
				const auto peer = _roundRobin.front();
				_roundRobin.pop_front();
				_roundRobin.push_back(peer);
				if (!skipped.contains(peer) && !_dispatchOne(peer, result))
				{
					skipped.insert(peer);
				}
			}
			return result;
		}

	public:
		explicit PublicationQueueTrait(std::size_t maximumPending = 16384, Clock::duration interval = std::chrono::milliseconds(50)) :
			_maximumPending(maximumPending),
			_interval(interval)
		{
			if (maximumPending == 0 || interval < Clock::duration::zero())
			{
				throw spk::Exception("Invalid publication queue configuration");
			}
		}
		PublicationQueueTrait(const PublicationQueueTrait &) = delete;
		PublicationQueueTrait &operator=(const PublicationQueueTrait &) = delete;
		virtual ~PublicationQueueTrait() = default;
	};
}
