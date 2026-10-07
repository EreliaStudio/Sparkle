#pragma once
#include "network/replication/request.hpp"
#include <map>
#include <optional>
namespace spk::Network
{
	class ObjectRequestHandlerTrait
	{
		struct History
		{
			spk::Message::RequestID lastRequest = 0;
			std::map<ObjectID, Request> pending;
		};
		std::map<PeerID, History> _requests;
		std::size_t _maximumPending;
		bool _requesting = false;

	protected:
		virtual void _requireRequestHandlerIdle() const;
		[[nodiscard]] virtual std::optional<SessionID> _requestSession(PeerID peer) const = 0;
		virtual void _requestObject(PeerID peer, const Request &request) = 0;
		[[nodiscard]] bool _isCurrentRequest(PeerID peer, const Request &request) const;
		void _completeRequest(PeerID peer, ObjectID object);
		void _cancelPeerRequests(PeerID peer);
		void _cancelObjectRequests(ObjectID object);

	public:
		explicit ObjectRequestHandlerTrait(std::size_t maximumPending = 16384);
		ObjectRequestHandlerTrait(const ObjectRequestHandlerTrait &) = delete;
		ObjectRequestHandlerTrait &operator=(const ObjectRequestHandlerTrait &) = delete;
		virtual ~ObjectRequestHandlerTrait() = default;
		[[nodiscard]] bool receiveRequest(PeerID peer, const Request &request);
	};
}
