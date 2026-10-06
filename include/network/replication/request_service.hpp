#pragma once

#include "failure.hpp"
#include "publisher.hpp"
#include "reply.hpp"

namespace spk::Network
{
	// Publisher outlives this service. One owner thread; no sender reentry.
	// Call close(peer) together with Publisher::close(peer).
	template <typename State>
	class RequestService final
	{
		using Key = std::pair<PeerID, ObjectID>;
		struct Entry
		{
			Request request;
			std::optional<Reply::Result> reply;
			bool scheduled = false;
		};
		Publisher<State> &_publisher;
		std::size_t _maximumEntries;
		std::map<Key, Entry> _entries;
		std::deque<Key> _order;
		bool _dispatching = false;
		void _check() const;
		Entry *_current(PeerID peer, const Request &request);
		void _schedule(PeerID peer, Entry &entry, Reply::Result result);

	public:
		explicit RequestService(Publisher<State> &publisher, std::size_t maximumEntries = 65536);
		// true => new application decision required. Duplicate/obsolete requests do not
		// start generation twice. Completed history is retained until peer close.
		[[nodiscard]] bool receive(PeerID peer, const Request &request);
		// For an asynchronous result, check correlation BEFORE publishing its state.
		[[nodiscard]] bool fulfill(PeerID peer, const Request &request, State state);
		// Use accept for a snapshot ALREADY present in the publisher (e.g. a cache hit).
		// Use fulfill for a new provider completion, to avoid publishing a stale result.
		[[nodiscard]] bool accept(PeerID peer, const Request &request);
		[[nodiscard]] bool reject(PeerID peer, const Request &request, Failure failure);
		void close(PeerID peer);
		// Call after publisher.dispatch(); replies use the SAME ordered transport.
		template <typename Sender>
		std::size_t dispatch(std::size_t maximumAttempts, Sender &&send);
	};
} // namespace spk::Network

#include "request_service.tpp"
