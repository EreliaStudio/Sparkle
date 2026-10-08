#include "network/trait/publication_dispatch_trait.hpp"
#include "exception.hpp"
#include <algorithm>
#include <set>
namespace spk::Network
{
	bool PublicationDispatchTrait::_dispatchOne(PeerID peer, DispatchResult &result)
	{
		const auto *pending = _front(peer);
		if (pending == nullptr)
		{
			return false;
		}
		try
		{
			if (_sendUpdate(peer, pending->update, pending->requestID))
			{
				_pop(peer);
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
	void PublicationDispatchTrait::_capturePublicationChanges()
	{
	}
	void PublicationDispatchTrait::_openQueue(PeerID peer)
	{
		PublicationQueueTrait::_openQueue(peer);
		try
		{
			_roundRobin.push_back(peer);
		} catch (...)
		{
			PublicationQueueTrait::_closeQueue(peer);
			throw;
		}
	}
	void PublicationDispatchTrait::_closeQueue(PeerID peer)
	{
		PublicationQueueTrait::_closeQueue(peer);
		std::erase(_roundRobin, peer);
	}
	PublicationDispatchTrait::DispatchResult PublicationDispatchTrait::_dispatchPublication(Clock::time_point now, std::size_t maximumAttempts)
	{
		DispatchResult result;
		if (now < _nextPublication || maximumAttempts == 0 || _roundRobin.empty())
		{
			return result;
		}
		_capturePublicationChanges();
		_nextPublication = now + _interval;
		std::set<PeerID> skipped;
		for (std::size_t count = 0; count < maximumAttempts && skipped.size() < _roundRobin.size(); ++count)
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
	PublicationDispatchTrait::PublicationDispatchTrait(std::size_t maximumPending, Clock::duration interval) :
		PublicationQueueTrait(maximumPending),
		_interval(interval)
	{
		if (interval < Clock::duration::zero())
		{
			throw spk::Exception("Invalid publication interval");
		}
	}
}
