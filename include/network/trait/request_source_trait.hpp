#pragma once

#include "network/replication/request_service.hpp"
#include "publication_source_trait.hpp"

namespace spk::Network
{
	// Optional acquisition capability. Derived hooks may complete immediately or
	// retain the request and complete later on the owner thread.
	template <typename State, typename Codec>
	class RequestSourceTrait : public PublicationSourceTrait<State, Codec>
	{
		using Base = PublicationSourceTrait<State, Codec>;
		RequestService<State> _requests;

	protected:
		virtual void _requestObject(PeerID peer, const Request &request) = 0;

		bool _receiveRequest(PeerID peer, const Request &request) final
		{
			if (!_requests.receive(peer, request))
			{
				return false;
			}
			try
			{
				_requestObject(peer, request);
			} catch (...)
			{
				(void)_requests.reject(peer, request, Failure::Transient);
				throw;
			}
			return true;
		}

		void _closeRequests(PeerID peer) final
		{
			_requests.close(peer);
		}

		void _dispatchReplies(std::size_t maximumAttempts) final
		{
			(void)_requests.dispatch(maximumAttempts, [this](PeerID peer, const Reply &reply) {
				return this->_sendMessage(peer, this->_protocolCodec().encode(reply));
			});
		}

	public:
		explicit RequestSourceTrait(spk::Message::Type type, typename Base::Configuration configuration = {}, std::size_t maximumRequests = 65536, std::size_t maximumBytes = 2 * 1024 * 1024) :
			Base(type, configuration, maximumBytes),
			_requests(this->_publication(), maximumRequests)
		{
		}

		// Snapshot-only providers can fulfill without constructing a live object.
		[[nodiscard]] bool fulfillRequest(PeerID peer, const Request &request, State state)
		{
			this->_requireIdle();
			return _requests.fulfill(peer, request, std::move(state));
		}

		[[nodiscard]] bool acceptRequest(PeerID peer, const Request &request)
		{
			this->_requireIdle();
			return _requests.accept(peer, request);
		}

		[[nodiscard]] bool rejectRequest(PeerID peer, const Request &request, Failure failure)
		{
			this->_requireIdle();
			return _requests.reject(peer, request, failure);
		}
	};
}
