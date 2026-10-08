#include "network/trait/publication_queue_trait.hpp"
#include "exception.hpp"
namespace spk::Network
{
	void PublicationQueueTrait::_openQueue(PeerID peer)
	{
		if (!_queues.try_emplace(peer).second)
		{
			throw spk::Exception("Duplicate publication queue");
		}
	}
	void PublicationQueueTrait::_closeQueue(PeerID peer)
	{
		_queues.erase(peer);
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
	const PublicationQueueTrait::Pending *PublicationQueueTrait::_front(PeerID peer) const
	{
		const auto found = _queues.find(peer);
		if (found == _queues.end())
		{
			throw spk::Exception("Unknown publication queue");
		}
		const auto &queue = found->second;
		return queue.order.empty() ? nullptr : &queue.pending.at(queue.order.front());
	}
	void PublicationQueueTrait::_pop(PeerID peer)
	{
		if (_front(peer) == nullptr)
		{
			throw spk::Exception("Empty publication queue");
		}
		auto &queue = _queues.at(peer);
		queue.pending.erase(queue.order.front());
		queue.order.pop_front();
	}
	PublicationQueueTrait::PublicationQueueTrait(std::size_t maximumPending) :
		_maximumPending(maximumPending)
	{
		if (maximumPending == 0)
		{
			throw spk::Exception("Invalid publication queue capacity");
		}
	}
}
