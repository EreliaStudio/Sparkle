#pragma once
#include "network/replication/types.hpp"
#include "network/types.hpp"
#include <map>
#include <optional>
namespace spk::Network
{
	// Negotiates decoded handshakes; the composing behavior owns peer sessions.
	class ServerHandshakeTrait
	{
		struct Connection
		{
			PeerID peer;
			SessionID token;
		};
		std::map<spk::ConnectionID, Connection> _connections;
		void _openHandshakeConnection(spk::ConnectionID connection, SessionID token);

	protected:
		[[nodiscard]] virtual SessionID _openHandshakePeer(PeerID peer) = 0;
		virtual void _closeHandshakePeer(PeerID peer) = 0;
		[[nodiscard]] virtual std::optional<SessionID> _findHandshakeSession(PeerID peer) const = 0;
		virtual void _sendHandshakeSession(spk::ConnectionID connection, SessionID token, SessionID session) = 0;
		void _receiveHello(spk::ConnectionID connection, SessionID token);
		void _closeHandshakeConnection(spk::ConnectionID connection);
		void _clearServerHandshake();
		[[nodiscard]] std::optional<PeerID> _handshakePeerID(spk::ConnectionID connection) const;
		[[nodiscard]] std::optional<spk::ConnectionID> _peerConnection(PeerID peer) const;

	public:
		ServerHandshakeTrait() = default;
		virtual ~ServerHandshakeTrait() = default;
		ServerHandshakeTrait(const ServerHandshakeTrait &) = delete;
		ServerHandshakeTrait &operator=(const ServerHandshakeTrait &) = delete;
	};
}
