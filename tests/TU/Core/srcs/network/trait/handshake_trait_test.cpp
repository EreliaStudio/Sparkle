#include "network/network_traits_test.hpp"

class HandshakeTraitTest : public NetworkTraitsTest
{
protected:
	class ClientHandshake : public spk::Network::ClientHandshakeTrait
	{
		void _closeHandshakeSession() override
		{
			session = {};
		}
		void _resetHandshakeSession(ID value) override
		{
			session = value;
		}
		bool _sendHello(ID token) override
		{
			tokens.push_back(token);
			if (throwSend)
			{
				throw spk::Exception("Hello send failed");
			}
			return !blocked;
		}

	public:
		using ClientHandshakeTrait::_beginClientHandshake;
		using ClientHandshakeTrait::_clearClientHandshake;
		using ClientHandshakeTrait::_receiveSession;
		using ClientHandshakeTrait::_treatClientHandshake;
		ID session;
		bool blocked = false, throwSend = false;
		std::vector<ID> tokens;
	};
	class ServerHandshake : public spk::Network::ServerHandshakeTrait, protected spk::Network::PeerSessionTrait
	{
		ID _openHandshakePeer(ID peer) override
		{
			return _openSession(peer);
		}
		void _closeHandshakePeer(ID peer) override
		{
			_closeSession(peer);
			closed.push_back(peer);
		}
		std::optional<ID> _findHandshakeSession(ID peer) const override
		{
			return _findSession(peer);
		}
		void _sendHandshakeSession(spk::ConnectionID, ID token, ID session) override
		{
			if (throwSend)
			{
				throw spk::Exception("Session send failed");
			}
			replies.push_back({token, session});
		}

	public:
		ServerHandshake() :
			PeerSessionTrait(2)
		{
		}
		using PeerSessionTrait::_closeSession;
		using ServerHandshakeTrait::_clearServerHandshake;
		using ServerHandshakeTrait::_closeHandshakeConnection;
		using ServerHandshakeTrait::_handshakePeerID;
		using ServerHandshakeTrait::_peerConnection;
		using ServerHandshakeTrait::_receiveHello;
		bool throwSend = false;
		std::vector<ID> closed;
		std::vector<Protocol::Handshake> replies;
	};
};

TEST_F(HandshakeTraitTest, ClientRetriesFailedHelloAndStopsAfterTransportAcceptance)
{
	ClientHandshake client;
	client._treatClientHandshake();
	EXPECT_TRUE(client.tokens.empty());
	client._beginClientHandshake();
	client.blocked = true;
	client._treatClientHandshake();
	const auto token = client.tokens.back();
	client.throwSend = true;
	EXPECT_THROW(client._treatClientHandshake(), spk::Exception);
	client.throwSend = false;
	client.blocked = false;
	client._treatClientHandshake();
	client._treatClientHandshake();
	ASSERT_EQ(client.tokens.size(), 3u);
	for (const auto value : client.tokens)
	{
		EXPECT_EQ(value, token);
	}
	EXPECT_TRUE(client._receiveSession(token, session));
	EXPECT_EQ(client.session, session);
}

TEST_F(HandshakeTraitTest, ClientRejectsStaleSessionAfterReconnectAndAfterClear)
{
	ClientHandshake client;
	client._beginClientHandshake();
	client._treatClientHandshake();
	const auto oldToken = client.tokens.back();
	ASSERT_TRUE(client._receiveSession(oldToken, session));
	client._beginClientHandshake();
	EXPECT_TRUE(client.session.isNull());
	EXPECT_FALSE(client._receiveSession(oldToken, session));
	client._treatClientHandshake();
	const auto token = client.tokens.back();
	EXPECT_NE(token, oldToken);
	EXPECT_TRUE(client._receiveSession(token, session));
	client._clearClientHandshake();
	EXPECT_TRUE(client.session.isNull());
	EXPECT_FALSE(client._receiveSession(token, session));
	client._treatClientHandshake();
	EXPECT_EQ(client.tokens.size(), 2u);
}

TEST_F(HandshakeTraitTest, ServerPreservesDuplicateHelloAndReplacesChangedToken)
{
	ServerHandshake server;
	const auto token = ID::generate();
	server._receiveHello(1, token);
	const auto peer = server._handshakePeerID(1);
	const auto first = server.replies.back().session;
	ASSERT_TRUE(peer);
	EXPECT_EQ(server._peerConnection(*peer), 1u);
	server._receiveHello(1, token);
	EXPECT_EQ(server._handshakePeerID(1), peer);
	EXPECT_EQ(server.replies.back().session, first);
	EXPECT_TRUE(server.closed.empty());
	server._receiveHello(1, ID::generate());
	EXPECT_NE(server._handshakePeerID(1), peer);
	EXPECT_NE(server.replies.back().session, first);
	EXPECT_EQ(server.closed, std::vector<ID>{*peer});
	EXPECT_FALSE(server._peerConnection(*peer));
}

TEST_F(HandshakeTraitTest, FailedServerReplyCanRetryWithoutReplacingSession)
{
	ServerHandshake server;
	const auto token = ID::generate();
	server.throwSend = true;
	EXPECT_THROW(server._receiveHello(1, token), spk::Exception);
	const auto peer = server._handshakePeerID(1);
	ASSERT_TRUE(peer);
	server.throwSend = false;
	server._receiveHello(1, token);
	EXPECT_EQ(server._handshakePeerID(1), peer);
	EXPECT_TRUE(server.closed.empty());
	ASSERT_EQ(server.replies.size(), 1u);
	EXPECT_EQ(server.replies.back().token, token);
}

TEST_F(HandshakeTraitTest, ClosedSessionRenegotiatesAndDisconnectClearsOnlyItsPeer)
{
	ServerHandshake server;
	const auto token = ID::generate();
	server._receiveHello(1, token);
	server._receiveHello(2, ID::generate());
	const auto oldPeer = *server._handshakePeerID(1);
	const auto other = server._handshakePeerID(2);
	server._closeSession(oldPeer);
	EXPECT_FALSE(server._handshakePeerID(1));
	server._receiveHello(1, token);
	EXPECT_NE(server._handshakePeerID(1), oldPeer);
	server._closeHandshakeConnection(1);
	EXPECT_FALSE(server._handshakePeerID(1));
	EXPECT_EQ(server._handshakePeerID(2), other);
	server._clearServerHandshake();
	EXPECT_FALSE(server._handshakePeerID(2));
	EXPECT_FALSE(server._peerConnection(*other));
}

TEST_F(HandshakeTraitTest, InvalidDecodedHandshakesDoNotChangeSessionState)
{
	ClientHandshake client;
	client._beginClientHandshake();
	client._treatClientHandshake();
	const auto token = client.tokens.back();
	EXPECT_THROW((void)client._receiveSession({}, session), spk::Exception);
	EXPECT_THROW((void)client._receiveSession(token, {}), spk::Exception);
	EXPECT_TRUE(client.session.isNull());
	ServerHandshake server;
	EXPECT_THROW(server._receiveHello(1, {}), spk::Exception);
	EXPECT_FALSE(server._handshakePeerID(1));
	EXPECT_TRUE(server.replies.empty());
}
