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
		void _check() const
		{
			if (_dispatching)
			{
				throw spk::Exception("Request service mutation during send");
			}
		}
		Entry *_current(PeerID peer, const Request &request)
		{
			auto found = _entries.find({peer, request.object});
			if (found == _entries.end() || found->second.request != request ||
				_publisher.session(peer) != request.session)
			{
				return nullptr;
			}
			return &found->second;
		}
		void _schedule(PeerID peer, Entry &entry, Reply::Result result)
		{
			entry.reply = result;
			if (entry.scheduled)
			{
				return;
			}
			entry.scheduled = true;
			_order.emplace_back(peer, entry.request.object);
		}

	public:
		explicit RequestService(Publisher<State> &publisher, std::size_t maximumEntries = 65536) :
			_publisher(publisher),
			_maximumEntries(maximumEntries)
		{
			if (maximumEntries == 0)
			{
				throw spk::Exception("Invalid request service capacity");
			}
		}
		// true => new application decision required. Duplicate/obsolete requests do not
		// start generation twice. Completed history is retained until peer close.
		[[nodiscard]] bool receive(PeerID peer, const Request &request)
		{
			_check();
			if (_publisher.session(peer) != request.session)
			{
				return false;
			}
			if (request.object.isNull())
			{
				throw spk::Exception("Null network identity");
			}
			if (request.attempt == 0)
			{
				throw spk::Exception("Invalid request attempt");
			}
			const Key key{peer, request.object};
			auto found = _entries.find(key);
			if (found != _entries.end())
			{
				if (request.attempt <= found->second.request.attempt)
				{
					return false;
				}
				found->second.request = request;
				found->second.reply.reset();
				return true;
			}
			if (_entries.size() == _maximumEntries)
			{
				throw spk::Exception("Request history full; close session");
			}
			_entries.emplace(key, Entry{request, std::nullopt, false});
			return true;
		}
		// For an asynchronous result, check correlation BEFORE publishing its state.
		[[nodiscard]] bool fulfill(PeerID peer, const Request &request, State state)
		{
			_check();
			const auto *entry = _current(peer, request);
			if (entry == nullptr || entry->reply.has_value())
			{
				return false;
			}
			_publisher.publish(request.object, std::move(state));
			return accept(peer, request);
		}
		// Use accept for a snapshot ALREADY present in the publisher (e.g. a cache hit).
		// Use fulfill for a new provider completion, to avoid publishing a stale result.
		[[nodiscard]] bool accept(PeerID peer, const Request &request)
		{
			_check();
			auto *entry = _current(peer, request);
			if (entry == nullptr || entry->reply.has_value())
			{
				return false;
			}
			_publisher.follow(peer, request.object);
			_schedule(peer, *entry, Reply::Result::Ready);
			return true;
		}
		[[nodiscard]] bool reject(PeerID peer, const Request &request, Failure failure)
		{
			_check();
			auto *entry = _current(peer, request);
			if (entry == nullptr || entry->reply.has_value())
			{
				return false;
			}
			_schedule(peer, *entry, failure == Failure::Transient ? Reply::Result::Retry : Reply::Result::Rejected);
			return true;
		}
		void close(PeerID peer)
		{
			_check();
			std::erase_if(_entries, [peer](const auto &entry) {
				return entry.first.first == peer;
			});
			std::erase_if(_order, [peer](const Key &key) {
				return key.first == peer;
			});
		}
		// Call after publisher.dispatch(); replies use the SAME ordered transport.
		template <typename Sender>
		std::size_t dispatch(std::size_t maximumAttempts, Sender &&send)
		{
			_check();
			_dispatching = true;
			struct Guard
			{
				bool &flag;
				~Guard()
				{
					flag = false;
				}
			} guard{_dispatching};
			std::size_t sent = 0;
			const auto count = std::min(maximumAttempts, _order.size());
			for (std::size_t index = 0; index < count; ++index)
			{
				const auto key = _order.front();
				_order.pop_front();
				auto &entry = _entries.at(key);
				entry.scheduled = false;
				if (!entry.reply || _publisher.session(key.first) != entry.request.session)
				{
					continue;
				}
				bool accepted = false;
				if (*entry.reply != Reply::Result::Ready ||
					_publisher.synchronized(key.first, key.second))
				{
					try
					{
						accepted =
							std::invoke(send, key.first, Reply{entry.request, *entry.reply});
					} catch (...)
					{ /* Sender owns diagnostics; retain reply and try other peers. */
					}
				}
				if (accepted)
				{
					++sent;
				}
				else
				{
					entry.scheduled = true;
					_order.push_back(key);
				}
			}
			return sent;
		}
	};
} // namespace spk::Network
