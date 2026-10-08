#include "network/trait/server_handshake_trait.hpp"
#include "exception.hpp"
namespace spk::Network
{
	void ServerHandshakeTrait::_openHandshakeConnection(spk::ConnectionID connection, SessionID token)
	{
		const auto peer = PeerID::generate();
		(void)_openHandshakePeer(peer);
		try
		{
			_connections.emplace(connection, Connection{peer, token});
		} catch (...)
		{
			_closeHandshakePeer(peer);
			throw;
		}
	}
	void ServerHandshakeTrait::_receiveHello(spk::ConnectionID connection, SessionID token)
	{
		if (token.isNull())
		{
			throw spk::Exception("Null handshake token");
		}
		auto found = _connections.find(connection);
		if (found != _connections.end() && (found->second.token != token || !_findHandshakeSession(found->second.peer)))
		{
			_closeHandshakeConnection(connection);
		}
		if (!_connections.contains(connection))
		{
			_openHandshakeConnection(connection, token);
		}
		const auto peer = _connections.at(connection).peer;
		const auto session = _findHandshakeSession(peer);
		if (!session || session->isNull())
		{
			throw spk::Exception("Handshake peer has no session");
		}
		_sendHandshakeSession(connection, token, *session);
	}
	void ServerHandshakeTrait::_closeHandshakeConnection(spk::ConnectionID connection)
	{
		auto found = _connections.find(connection);
		if (found != _connections.end())
		{
			_closeHandshakePeer(found->second.peer);
			_connections.erase(found);
		}
	}
	void ServerHandshakeTrait::_clearServerHandshake()
	{
		while (!_connections.empty())
		{
			_closeHandshakeConnection(_connections.begin()->first);
		}
	}
	std::optional<PeerID> ServerHandshakeTrait::_handshakePeerID(spk::ConnectionID connection) const
	{
		auto found = _connections.find(connection);
		return found == _connections.end() || !_findHandshakeSession(found->second.peer) ? std::nullopt : std::optional{found->second.peer};
	}
	std::optional<spk::ConnectionID> ServerHandshakeTrait::_peerConnection(PeerID peer) const
	{
		for (const auto &[connection, binding] : _connections)
		{
			if (binding.peer == peer)
			{
				return connection;
			}
		}
		return std::nullopt;
	}
}
