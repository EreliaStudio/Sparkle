#include "network/replication/request_queue.hpp"

#include "exception.hpp"
#include <algorithm>

namespace spk::Network
{
	void RequestQueue::_retry(Entry &entry, Failure failure, Clock::time_point now)
	{
		if (failure == Failure::Permanent ||
			entry.attempts >= _configuration.maximumAttempts)
		{
			entry.status = Status::Failed;
			return;
		}
		entry.status = Status::Waiting;
		entry.deadline = now + entry.delay;
		entry.delay = entry.delay >= _configuration.maximumDelay / 2
						  ? _configuration.maximumDelay
						  : entry.delay * 2;
	}

	RequestQueue::RequestQueue() :
		RequestQueue(Configuration{})
	{
	}

	RequestQueue::RequestQueue(Configuration configuration) :
		_configuration(configuration)
	{
		if (configuration.timeout <= Clock::duration::zero() ||
			configuration.initialDelay <= Clock::duration::zero() ||
			configuration.maximumDelay < configuration.initialDelay ||
			configuration.maximumAttempts == 0 || configuration.maximumRequests == 0)
		{
			throw spk::Exception("Invalid request retry policy");
		}
	}

	std::vector<ObjectID> RequestQueue::reset(SessionID session)
	{
		if (session.isNull())
		{
			throw spk::Exception("Null network identity");
		}
		if (session == _session)
		{
			return {};
		}
		auto desired = std::vector<ObjectID>(_order.begin(), _order.end());
		_entries.clear();
		_order.clear();
		_session = session;
		return desired;
	}

	void RequestQueue::close()
	{
		_entries.clear();
		_order.clear();
		_session = {};
	}

	void RequestQueue::request(ObjectID id, Clock::time_point now)
	{
		if (id.isNull())
		{
			throw spk::Exception("Null network identity");
		}
		if (_session.isNull())
		{
			throw spk::Exception("Request session not initialized");
		}
		if (_entries.contains(id))
		{
			return;
		}
		if (_entries.size() == _configuration.maximumRequests)
		{
			throw spk::Exception("Request capacity reached");
		}
		_entries.emplace(
			id,
			Entry{
				{_session, id, 0}, Status::Waiting, 0, now, _configuration.initialDelay});
		_order.push_back(id);
	}

	void RequestQueue::release(ObjectID id)
	{
		_entries.erase(id);
		std::erase(_order, id);
	}

	std::optional<RequestQueue::Status> RequestQueue::status(ObjectID id) const
	{
		auto found = _entries.find(id);
		if (found == _entries.end())
		{
			return std::nullopt;
		}
		return found->second.status;
	}

	std::vector<Request> RequestQueue::due(Clock::time_point now, std::size_t maximum)
	{
		std::vector<Request> result;
		const auto count = _order.size();
		for (std::size_t index = 0; index < count && result.size() < maximum; ++index)
		{
			const auto id = _order.front();
			_order.pop_front();
			_order.push_back(id);
			auto &entry = _entries.at(id);
			if (entry.status == Status::Pending && now >= entry.deadline)
			{
				_retry(entry, Failure::Transient, now);
			}
			if (entry.status != Status::Waiting || now < entry.deadline)
			{
				continue;
			}
			entry.request.attempt = _sequence.next();
			++entry.attempts;
			entry.status = Status::Pending;
			entry.deadline = now + _configuration.timeout;
			result.push_back(entry.request);
		}
		return result;
	}

	bool RequestQueue::receive(const Reply &reply, Clock::time_point now)
	{
		auto found = _entries.find(reply.request.object);
		if (found == _entries.end() || found->second.request != reply.request ||
			found->second.status != Status::Pending)
		{
			return false;
		}
		if (reply.result > Reply::Result::Rejected)
		{
			throw spk::Exception("Invalid acquisition reply");
		}
		if (reply.result == Reply::Result::Ready)
		{
			found->second.status = Status::Ready;
		}
		else
		{
			_retry(found->second, reply.result == Reply::Result::Retry ? Failure::Transient : Failure::Permanent, now);
		}
		return true;
	}
}
