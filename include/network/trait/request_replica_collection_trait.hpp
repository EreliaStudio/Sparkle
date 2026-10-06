#pragma once

#include "network/replication/request_queue.hpp"
#include "replica_collection_trait.hpp"

namespace spk::Network
{
	template <typename State, typename Codec>
	class RequestReplicaCollectionTrait : public ReplicaCollectionTrait<State, Codec>
	{
		using Base = ReplicaCollectionTrait<State, Codec>;
		RequestQueue _requests;
		bool _dispatching = false;

		[[nodiscard]] bool _sendRequest(const Request &request, Clock::time_point now)
		{
			try
			{
				if (_sendMessage(this->_protocolCodec().encode(request)))
				{
					return true;
				}
			} catch (...)
			{
			}
			(void)_requests.receive(Reply{request, Reply::Result::Retry}, now);
			return false;
		}

	protected:
		[[nodiscard]] virtual bool _sendMessage(const spk::Message &message) = 0;
		void _requireTransportIdle() const final
		{
			if (_dispatching)
			{
				throw spk::Exception("Collection mutation during request dispatch");
			}
		}

		void _resetRequests(SessionID session) final
		{
			if (session.isNull())
			{
				_requests.close();
			}
			else
			{
				(void)_requests.reset(session);
			}
		}

		bool _receiveReply(const Reply &reply, Clock::time_point now) final
		{
			if (reply.result == Reply::Result::Ready && !this->_hasReplica(reply.request.object))
			{
				return false;
			}
			return _requests.receive(reply, now);
		}

	public:
		explicit RequestReplicaCollectionTrait(spk::Message::Type type, RequestQueue::Configuration configuration = {}, std::size_t maximumTracked = 65536, std::size_t maximumBytes = 2 * 1024 * 1024) :
			Base(type, maximumTracked, maximumBytes),
			_requests(configuration)
		{
		}

		void requestObject(ObjectID id, Clock::time_point now = Clock::now())
		{
			this->_requireIdle();
			_requests.request(id, now);
		}

		// Local cancellation only; remote interest removal is application policy.
		void cancelRequest(ObjectID id)
		{
			this->_requireIdle();
			_requests.release(id);
		}

		[[nodiscard]] std::optional<RequestQueue::Status> requestStatus(ObjectID id) const
		{
			return _requests.status(id);
		}

		virtual std::size_t dispatchRequests(Clock::time_point now, std::size_t maximumAttempts = 64) final
		{
			this->_requireIdle();
			OperationGuard guard(_dispatching);
			std::size_t sent = 0;
			for (const auto &request : _requests.due(now, maximumAttempts))
			{
				if (_sendRequest(request, now))
				{
					++sent;
				}
			}
			return sent;
		}
	};
}
