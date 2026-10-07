#include "network/trait/publication_queue_trait.hpp"

namespace spk::Network
{
	bool PublicationQueueTrait::_dispatchOne(PeerID peer, DispatchResult &result)
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
	void PublicationQueueTrait::_capturePublicationChanges()
	{
	}
	void PublicationQueueTrait::_openQueue(PeerID peer)
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
	void PublicationQueueTrait::_closeQueue(PeerID peer)
	{
		_queues.erase(peer);
		std::erase(_roundRobin, peer);
	}
	void PublicationQueueTrait::_room(PeerID peer, ObjectID id) const
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
	void PublicationQueueTrait::_queue(PeerID peer, Update<spk::Message> update, spk::Message::RequestID requestID)
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
	PublicationQueueTrait::DispatchResult PublicationQueueTrait::_dispatchPublication(Clock::time_point now, std::size_t maximumAttempts)
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
	PublicationQueueTrait::PublicationQueueTrait(std::size_t maximumPending, Clock::duration interval) :
		_maximumPending(maximumPending),
		_interval(interval)
	{
		if (maximumPending == 0 || interval < Clock::duration::zero())
		{
			throw spk::Exception("Invalid publication queue configuration");
		}
	}
}
