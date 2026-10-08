#include "network/network_binding_test.hpp"

class ConnectionObservationTraitTest : public NetworkBindingTest
{
protected:
	class ServerObserver : public spk::Network::ServerConnectionObservationTrait
	{
		void _onServerConnectionOpened(spk::ConnectionID id) override
		{
			EXPECT_EQ(std::this_thread::get_id(), owner);
			opened.push_back(id);
		}
		void _onServerConnectionClosed(spk::ConnectionID id) override
		{
			EXPECT_EQ(std::this_thread::get_id(), owner);
			closed.push_back(id);
		}

	public:
		using ServerConnectionObservationTrait::_connectionLive;
		using ServerConnectionObservationTrait::_observeServerConnections;
		using ServerConnectionObservationTrait::_releaseServerObservation;
		using ServerConnectionObservationTrait::_synchronizeServerConnections;
		std::thread::id owner = std::this_thread::get_id();
		std::vector<spk::ConnectionID> opened, closed;
	};
	class ClientObserver : public spk::Network::ClientConnectionObservationTrait
	{
		void _onClientConnectionChanged() override
		{
			EXPECT_EQ(std::this_thread::get_id(), owner);
			if (fail)
			{
				throw spk::Exception("Observation failure");
			}
			++changes;
		}

	public:
		using ClientConnectionObservationTrait::_clientConnectionObserved;
		using ClientConnectionObservationTrait::_observeClientConnection;
		using ClientConnectionObservationTrait::_releaseClientObservation;
		using ClientConnectionObservationTrait::_synchronizeClientConnection;
		std::thread::id owner = std::this_thread::get_id();
		int changes = 0;
		bool fail = false;
	};
};

TEST_F(ConnectionObservationTraitTest, ObservesConnectionsOnOwnerThreadWithoutMessageBindings)
{
	Server server;
	Client client;
	ServerObserver serverObserver;
	ClientObserver clientObserver;
	serverObserver._observeServerConnections(server);
	clientObserver._observeClientConnection(client);
	server.start(0);
	client.connect("127.0.0.1", server.port());
	EXPECT_EQ(clientObserver.changes, 0);
	EXPECT_TRUE(serverObserver.opened.empty());
	clientObserver._synchronizeClientConnection();
	EXPECT_EQ(clientObserver.changes, 1);
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		serverObserver._synchronizeServerConnections();
		return serverObserver.opened.size() == 1;
	}));
	const auto connection = serverObserver.opened.front();
	EXPECT_TRUE(serverObserver._connectionLive(connection));
	client.disconnect();
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		serverObserver._synchronizeServerConnections();
		return serverObserver.closed.size() == 1;
	}));
	EXPECT_EQ(serverObserver.closed.front(), connection);
	EXPECT_FALSE(serverObserver._connectionLive(connection));
	clientObserver._synchronizeClientConnection();
	EXPECT_EQ(clientObserver.changes, 2);
}

TEST_F(ConnectionObservationTraitTest, ReconnectWithoutObservationStillInvalidatesClientEdition)
{
	Server server;
	Client client;
	ClientObserver observer;
	observer._observeClientConnection(client);
	server.start(0);
	client.connect("127.0.0.1", server.port());
	observer._synchronizeClientConnection();
	ASSERT_TRUE(observer._clientConnectionObserved());
	const auto changes = observer.changes;
	client.disconnect();
	client.connect("127.0.0.1", server.port());
	EXPECT_FALSE(observer._clientConnectionObserved());
	observer._synchronizeClientConnection();
	EXPECT_EQ(observer.changes, changes + 1);
	EXPECT_TRUE(observer._clientConnectionObserved());
	observer._releaseClientObservation();
	client.disconnect();
	observer._synchronizeClientConnection();
	EXPECT_EQ(observer.changes, changes + 1);
	EXPECT_FALSE(observer._clientConnectionObserved());
}

TEST_F(ConnectionObservationTraitTest, FailedClientHookRetriesAndStoppedTransportMayDieFirst)
{
	ClientObserver observer;
	{
		Client client;
		observer._observeClientConnection(client);
		observer.fail = true;
		EXPECT_THROW(observer._synchronizeClientConnection(), spk::Exception);
		EXPECT_FALSE(observer._clientConnectionObserved());
		observer.fail = false;
		observer._synchronizeClientConnection();
		EXPECT_TRUE(observer._clientConnectionObserved());
		EXPECT_EQ(observer.changes, 1);
	}
	EXPECT_FALSE(observer._clientConnectionObserved());
	EXPECT_NO_THROW(observer._releaseClientObservation());
	EXPECT_NO_THROW(observer._synchronizeClientConnection());
}

TEST_F(ConnectionObservationTraitTest, ServerObservationMustStartBeforeConnectionsAndCanBeReleased)
{
	Server server;
	ServerObserver observer;
	server.start(0);
	EXPECT_THROW(observer._observeServerConnections(server), spk::Exception);
	server.stop();
	observer._observeServerConnections(server);
	observer._releaseServerObservation();
	EXPECT_NO_THROW(observer._synchronizeServerConnections());
	Client client;
	server.start(0);
	client.connect("127.0.0.1", server.port());
	observer._synchronizeServerConnections();
	EXPECT_TRUE(observer.opened.empty());
}
