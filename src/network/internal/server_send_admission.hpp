#pragma once

#include "exception.hpp"
#include "network/internal/exception_reporting.hpp"
#include "network/internal/server_send_batch.hpp"
#include "network/internal/transfer_completion.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace spk::NetworkInternal
{
	template <typename TSession>
	class ServerSendAdmission final
	{
	private:
		using Sessions = std::unordered_map<ConnectionID, std::shared_ptr<TSession>>;
		using Recipient = ServerSendBatch::Recipient;

		struct Candidate final
		{
			Recipient *recipient;
			std::shared_ptr<TSession> session;
		};

		ServerSendBatch &_batch;
		const Sessions &_sessions;
		CommandBudget &_budget;
		const std::function<std::vector<Connection>()> &_snapshot;
		std::vector<Candidate> _candidates;
		std::vector<ConnectionID> _overflowed;
		std::vector<std::pair<Recipient *, Transfer::Reason>> _declined;

		[[nodiscard]] bool _ready(const Connection &connection) const
		{
			const auto it = _sessions.find(connection.id());
			return connection.isConnected() && it != _sessions.end() && it->second->handshake.ready();
		}

		void _discover()
		{
			if (!_batch.allRecipients)
			{
				return;
			}
			const auto connections = _snapshot();
			if (connections.empty() && _batch.policy != Connection::OverflowPolicy::SkipRecipient)
			{
				throw Exception("Cannot broadcast without connected recipients.");
			}
			_batch.recipients.reserve(connections.size());
			for (const Connection &connection : connections)
			{
				if (!_ready(connection))
				{
					if (_batch.policy == Connection::OverflowPolicy::SkipRecipient)
					{
						continue;
					}
					throw Exception("Cannot broadcast while a recipient is pending or disconnected.");
				}
				_batch.recipients.push_back({connection, Transfer{}});
			}
		}

		void _decline(Recipient &recipient, ConnectionID id)
		{
			_declined.push_back({&recipient, Transfer::Reason::QueueOverflow});
			if (_batch.policy == Connection::OverflowPolicy::DisconnectRecipient)
			{
				_overflowed.push_back(id);
			}
		}

		void _reserveOne(Recipient &recipient)
		{
			const auto it = _sessions.find(recipient.connection.id());
			if (it == _sessions.end() || !it->second->handshake.ready())
			{
				throw Exception("Cannot send to an invalid, pending or disconnected recipient.");
			}
			const auto &session = it->second;
			const std::size_t logical = std::max<std::size_t>(1, _batch.message.size());
			const std::size_t frame = OutgoingReservation::frameBytes(_batch.message.size(), session->handshake.maximumPageBytes());
			const std::size_t capacity = recipient.connection.maxQueuedBytes();
			const std::size_t used = session->reservedOutgoingBytes;
			if (used > capacity || logical > capacity - used || frame > capacity - used - logical)
			{
				_decline(recipient, session->id);
				return;
			}
			auto reservation = OutgoingReservation::tryReserve(_budget, _batch.message.size(), session->handshake.maximumPageBytes());
			if (!reservation.has_value())
			{
				_decline(recipient, session->id);
				return;
			}
			recipient.reservation = std::move(*reservation);
			_candidates.push_back({&recipient, session});
		}

		void _rejectAtomic()
		{
			if (_declined.empty() || _batch.policy != Connection::OverflowPolicy::Reject)
			{
				return;
			}
			for (Candidate &candidate : _candidates)
			{
				candidate.recipient->reservation = {};
			}
			_batch.rejectAll(_declined);
			throw Exception("Insufficient capacity for atomic network broadcast admission.");
		}

		static void _release(TSession &session, Transfer::ID id) noexcept
		{
			const auto it = session.outgoingReservations.find(id);
			if (it == session.outgoingReservations.end())
			{
				return;
			}
			session.reservedOutgoingBytes -= it->second.bytes();
			session.outgoingReservations.erase(it);
		}

		void _rollback() noexcept
		{
			for (Candidate &candidate : _candidates)
			{
				if (candidate.recipient->admitted)
				{
					(void)candidate.session->transfers.fail(candidate.recipient->transfer.id(), Transfer::Status::Rejected, Transfer::Reason::QueueOverflow);
					_release(*candidate.session, candidate.recipient->transfer.id());
				}
				candidate.recipient->reservation = {};
			}
		}

		template <typename TQueue>
		void _commit(TQueue &queue)
		{
			try
			{
				for (Candidate &candidate : _candidates)
				{
					candidate.session->transfers.setMaxPendingBytes(candidate.recipient->connection.maxQueuedBytes());
					queue(candidate.session, *candidate.recipient);
					candidate.recipient->admitted = true;
				}
			} catch (...)
			{
				_rollback();
				throw Exception("Unable to commit atomic network send admission.", std::current_exception());
			}
		}

		template <typename TFlush, typename TDisconnect>
		void _write(TFlush &flush, TDisconnect &disconnect)
		{
			for (const Candidate &candidate : _candidates)
			{
				try
				{
					flush(candidate.session);
				} catch (...)
				{
					reportCurrentException("Unable to write an admitted network send.");
					disconnect(candidate.session->id);
				}
			}
			for (ConnectionID id : _overflowed)
			{
				disconnect(id);
			}
			for (const auto &[recipient, reason] : _declined)
			{
				(void)TransferCompletion::complete(recipient->transfer, Transfer::Status::Rejected, reason);
			}
		}

	public:
		ServerSendAdmission(
			ServerSendBatch &batch,
			const Sessions &sessions,
			CommandBudget &budget,
			const std::function<std::vector<Connection>()> &snapshot) :
			_batch(batch),
			_sessions(sessions),
			_budget(budget),
			_snapshot(snapshot)
		{
		}

		template <typename TQueue, typename TFlush, typename TDisconnect>
		void run(TQueue queue, TFlush flush, TDisconnect disconnect)
		{
			_discover();
			for (Recipient &recipient : _batch.recipients)
			{
				_reserveOne(recipient);
			}
			_rejectAtomic();
			_commit(queue);
			_write(flush, disconnect);
		}
	};
}
