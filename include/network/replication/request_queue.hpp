#pragma once

#include "failure.hpp"
#include "reply.hpp"
#include "sequence.hpp"
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
		void _retry(Entry &entry, Failure failure, Clock::time_point now);

	public:
		RequestQueue();
		explicit RequestQueue(Configuration configuration);
		// Explicit transition: desired IDs are returned so the owner can choose to
		// re-request.
		[[nodiscard]] std::vector<ObjectID> reset(SessionID session);
		void close();
		void request(ObjectID id, Clock::time_point now);
		// Cancels the LOCAL transaction. Send application interest-release separately.
		// Provider completions for a released transaction are ignored by this queue.
		void release(ObjectID id);
		[[nodiscard]] std::optional<Status> status(ObjectID id) const;
		// New attempt IDs prevent an earlier response completing a later retry.
		[[nodiscard]] std::vector<Request> due(Clock::time_point now, std::size_t maximum = 64);
		[[nodiscard]] bool receive(const Reply &reply, Clock::time_point now);
	};
} // namespace spk::Network
