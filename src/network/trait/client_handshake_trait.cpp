#include "network/trait/client_handshake_trait.hpp"
#include "exception.hpp"
namespace spk::Network
{
	void ClientHandshakeTrait::_beginClientHandshake()
	{
		_closeHandshakeSession();
		_handshakeToken = SessionID::generate();
		_helloSent = false;
	}
	void ClientHandshakeTrait::_treatClientHandshake()
	{
		if (!_helloSent && !_handshakeToken.isNull())
		{
			_helloSent = _sendHello(_handshakeToken);
		}
	}
	void ClientHandshakeTrait::_clearClientHandshake()
	{
		_closeHandshakeSession();
		_handshakeToken = {};
		_helloSent = false;
	}
	bool ClientHandshakeTrait::_receiveSession(SessionID token, SessionID session)
	{
		if (token.isNull() || session.isNull())
		{
			throw spk::Exception("Null handshake identity");
		}
		if (token != _handshakeToken)
		{
			return false;
		}
		_resetHandshakeSession(session);
		return true;
	}
}
