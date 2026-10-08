#pragma once
#include "network/replication/types.hpp"
namespace spk::Network
{
	// Negotiates decoded session replies independently of message encoding and transport.
	class ClientHandshakeTrait
	{
		SessionID _handshakeToken;
		bool _helloSent = false;

	protected:
		virtual void _closeHandshakeSession() = 0;
		virtual void _resetHandshakeSession(SessionID session) = 0;
		[[nodiscard]] virtual bool _sendHello(SessionID token) = 0;
		void _beginClientHandshake();
		void _treatClientHandshake();
		void _clearClientHandshake();
		[[nodiscard]] bool _receiveSession(SessionID token, SessionID session);

	public:
		ClientHandshakeTrait() = default;
		virtual ~ClientHandshakeTrait() = default;
		ClientHandshakeTrait(const ClientHandshakeTrait &) = delete;
		ClientHandshakeTrait &operator=(const ClientHandshakeTrait &) = delete;
	};
}
