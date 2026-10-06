#pragma once

#include "failure.hpp"
#include "reply.hpp"
#include "sequence.hpp"
#include <algorithm>
#include <cstddef>
#include <deque>
#include <map>
#include <optional>
#include <vector>

namespace spk::Network
{
	// Ready follows the initial state on the same ordered transport.
	// All operations run on one owner thread.
	class RequestQueue final
	{
	public:
		enum class Status
		{
			Waiting,
			Pending,
			Ready,
			Failed
		};
		struct Configuration
		{
			Clock::duration timeout = std::chrono::seconds(15);
			Clock::duration initialDelay = std::chrono::seconds(1);
			Clock::duration maximumDelay = std::chrono::seconds(8);
			std::uint32_t maximumAttempts = 8;
			std::size_t maximumRequests = 16384;
		};

	private:
		struct Entry
		{
			Request request;
			Status status = Status::Waiting;
			std::uint32_t attempts = 0;
			Clock::time_point deadline;
			Clock::duration delay;
		};
		Configuration _configuration;
		SessionID _session;
		Sequence _sequence;
		std::map<ObjectID, Entry> _entries;
		std::deque<ObjectID> _order;
		void _retry(Entry &entry, Failure failure, Clock::time_point now)
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

	public:
		RequestQueue() :
			RequestQueue(Configuration{})
		{
		}
		explicit RequestQueue(Configuration configuration) :
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
		// Explicit transition: desired IDs are returned so the owner can choose to
		// re-request.
		[[nodiscard]] std::vector<ObjectID> reset(SessionID session)
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
		void request(ObjectID id, Clock::time_point now)
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
		// Cancels the LOCAL transaction. Send application interest-release separately.
		// Provider completions for a released transaction are ignored by this queue.
		void release(ObjectID id)
		{
			_entries.erase(id);
			std::erase(_order, id);
		}
		[[nodiscard]] std::optional<Status> status(ObjectID id) const
		{
			auto found = _entries.find(id);
			if (found == _entries.end())
			{
				return std::nullopt;
			}
			return found->second.status;
		}
		// New attempt IDs prevent an earlier response completing a later retry.
		[[nodiscard]] std::vector<Request> due(Clock::time_point now, std::size_t maximum = 64)
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
		[[nodiscard]] bool receive(const Reply &reply, Clock::time_point now)
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
	};
} // namespace spk::Network
