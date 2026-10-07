#include "network/network_binding_test.hpp"

TEST_F(NetworkBindingTest, BindAcquiresUpdatesDestroysAndCleansUpThroughRealTransport)
{
	Server server;
	Client client;
	Source source;
	Replicas replicas;
	Object entity;
	entity.change(12);
	source.registerObject(object, entity);
	source.bind(server);
	replicas.bind(client);
	server.start(0);
	client.connect("127.0.0.1", server.port());
	auto pump = [&] {
		client.treatMessages();
		server.treatMessages();
		source.dispatch(Clock::now());
		client.treatMessages();
	};
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return replicas.isSynchronized();
	}));
	ASSERT_TRUE(replicas.requestObject(object));
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return replicas.objects.contains(object);
	}));
	EXPECT_EQ(replicas.objects.at(object).value, 12);
	EXPECT_EQ(replicas.requestStatus(object), Replicas::RequestStatus::Ready);
	entity.change(25);
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return replicas.objects.at(object).value == 25;
	}));
	EXPECT_EQ(replicas.creations, 1);
	source.destroyObject(object);
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return replicas.objects.empty();
	}));
	EXPECT_FALSE(replicas.requestStatus(object));
	client.disconnect();
	pump();
	EXPECT_FALSE(replicas.isSynchronized());
	server.stop();
}

TEST_F(NetworkBindingTest, IndependentChannelsDoNotDecodeEachOthersState)
{
	Server server;
	Client client;
	Source first(42), second(43);
	Replicas a(42), b(43);
	Object one, two;
	one.change(10);
	two.change(20);
	first.registerObject(object, one);
	second.registerObject(object, two);
	first.bind(server);
	second.bind(server);
	a.bind(client);
	b.bind(client);
	server.start(0);
	client.connect("127.0.0.1", server.port());
	auto pump = [&] {
		client.treatMessages();
		server.treatMessages();
		first.dispatch(Clock::now());
		second.dispatch(Clock::now());
		client.treatMessages();
	};
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return a.isSynchronized() && b.isSynchronized();
	}));
	ASSERT_TRUE(a.requestObject(object));
	ASSERT_TRUE(b.requestObject(object));
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return a.objects.contains(object) && b.objects.contains(object);
	}));
	EXPECT_EQ(a.objects.at(object).value, 10);
	EXPECT_EQ(b.objects.at(object).value, 20);
	first.destroyObject(object);
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return a.objects.empty();
	}));
	EXPECT_EQ(b.objects.at(object).value, 20);
	client.disconnect();
	server.stop();
	pump();
	EXPECT_TRUE(b.objects.empty());
}

TEST_F(NetworkBindingTest, BindAfterConnectAndRebindRejectOldHandshakeAndState)
{
	Server server;
	Client client;
	Source source;
	Replicas replicas;
	Object entity;
	source.registerObject(object, entity);
	source.bind(server);
	server.start(0);
	client.connect("127.0.0.1", server.port());
	replicas.bind(client);
	std::vector<spk::Message> observed;
	auto observer = client.messageDispatcher().subscribeTo(42, [&](const auto &message) {
		observed.push_back(message);
	});
	auto pump = [&] {
		client.treatMessages();
		server.treatMessages();
		source.dispatch(Clock::now());
		client.treatMessages();
	};
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return replicas.isSynchronized();
	}));
	ASSERT_TRUE(replicas.requestObject(object));
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return replicas.objects.contains(object);
	}));
	const auto old = observed;
	replicas.bind(client); // Same binding is idempotent.
	EXPECT_EQ(replicas.creations, 1);
	replicas.unbind();
	EXPECT_FALSE(replicas.isBound());
	EXPECT_TRUE(replicas.objects.empty());
	replicas.bind(client);
	for (const auto &message : old)
	{
		client.messages().publish(message);
	}
	client.treatMessages();
	EXPECT_FALSE(replicas.isSynchronized());
	EXPECT_TRUE(replicas.objects.empty());
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return replicas.isSynchronized();
	}));
	for (const auto &message : old)
	{
		client.messages().publish(message);
	}
	client.treatMessages();
	EXPECT_TRUE(replicas.objects.empty());
	ASSERT_TRUE(replicas.requestObject(object));
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return replicas.objects.contains(object);
	}));
	client.disconnect();
	server.stop();
}

TEST_F(NetworkBindingTest, ReconnectWithoutIntermediateTreatmentClearsOldReplicas)
{
	Server server;
	Client client;
	Source source;
	Replicas replicas;
	Object entity;
	source.registerObject(object, entity);
	source.bind(server);
	replicas.bind(client);
	server.start(0);
	client.connect("127.0.0.1", server.port());
	auto pump = [&] {
		client.treatMessages();
		server.treatMessages();
		source.dispatch(Clock::now());
		client.treatMessages();
	};
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return replicas.isSynchronized();
	}));
	ASSERT_TRUE(replicas.requestObject(object));
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return replicas.objects.contains(object);
	}));
	client.disconnect();
	client.connect("127.0.0.1", server.port());
	EXPECT_FALSE(replicas.isSynchronized());
	client.treatMessages();
	EXPECT_TRUE(replicas.objects.empty());
	EXPECT_FALSE(replicas.requestStatus(object));
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return replicas.isSynchronized();
	}));
	ASSERT_TRUE(replicas.requestObject(object));
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return replicas.objects.contains(object);
	}));
	EXPECT_EQ(replicas.creations, 2);
	server.stop();
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		client.treatMessages();
		return !client.isConnected() && replicas.objects.empty();
	}));
	client.disconnect();
}

TEST_F(NetworkBindingTest, DestructionResignsContractsAndEitherStoppedTransportLifetimeIsSafe)
{
	Client client;
	Server server;
	{
		Source source;
		Replicas replicas;
		source.bind(server);
		replicas.bind(client);
	}
	// These malformed frames would throw if the destroyed collections were called.
	client.messages().publish(std::move(spk::Message::Writer(42)).build());
	server.messages().publish({1, std::move(spk::Message::Writer(42)).build()});
	EXPECT_NO_THROW(client.treatMessages());
	EXPECT_NO_THROW(server.treatMessages());
	Source survivor;
	Replicas other;
	{
		Server temporaryServer;
		Client temporaryClient;
		survivor.bind(temporaryServer);
		other.bind(temporaryClient);
	}
	EXPECT_FALSE(survivor.isBound());
	EXPECT_FALSE(other.isBound());
	EXPECT_FALSE(other.isSynchronized());
	EXPECT_NO_THROW(survivor.unbind());
	EXPECT_NO_THROW(other.unbind());
}

TEST_F(NetworkBindingTest, ServerBindingRequiresRegistrationBeforeAcceptingConnections)
{
	Server server;
	Source source;
	server.start(0);
	EXPECT_THROW(source.bind(server), spk::Exception);
	server.stop();
	source.bind(server);
	source.bind(server);
	EXPECT_TRUE(source.isBound());
	source.unbind();
	EXPECT_FALSE(source.isBound());
}

TEST_F(NetworkBindingTest, ServerCanPublishWithoutAClientAcquisitionRequest)
{
	Server server;
	Client client;
	Source source;
	Replicas replicas;
	Object entity;
	entity.change(7);
	source.registerObject(object, entity);
	source.bind(server);
	replicas.bind(client);
	std::optional<spk::ConnectionID> connection;
	auto observer = server.messageDispatcher().subscribeTo(42, [&](const auto &received) {
		connection = received.emitter;
	});
	server.start(0);
	client.connect("127.0.0.1", server.port());
	auto pump = [&] {
		client.treatMessages();
		server.treatMessages();
		source.dispatch(Clock::now());
		client.treatMessages();
	};
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return replicas.isSynchronized();
	}));
	ASSERT_TRUE(connection);
	const auto peer = source.peerID(*connection);
	ASSERT_TRUE(peer);
	source.follow(*peer, object);
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return replicas.objects.contains(object);
	}));
	EXPECT_EQ(replicas.objects.at(object).value, 7);
	EXPECT_FALSE(replicas.requestStatus(object));
	source.forget(*peer, object);
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return replicas.objects.empty();
	}));
	client.disconnect();
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		server.treatMessages();
		return !source.peerID(*connection);
	}));
}

TEST_F(NetworkBindingTest, DisconnectInvalidatesDeferredProviderCompletion)
{
	class DeferredSource : public Source
	{
		void _requestObject(ID peer, const spk::Network::Request &request) override
		{
			pending.emplace_back(peer, request);
		}

	public:
		std::vector<std::pair<ID, spk::Network::Request>> pending;
	};
	Server server;
	Client client;
	DeferredSource source;
	Replicas replicas;
	source.bind(server);
	replicas.bind(client);
	server.start(0);
	client.connect("127.0.0.1", server.port());
	std::optional<spk::ConnectionID> connection;
	auto observer = server.messageDispatcher().subscribeTo(42, [&](const auto &received) {
		connection = received.emitter;
	});
	auto pump = [&] {
		client.treatMessages();
		server.treatMessages();
		source.dispatch(Clock::now());
		client.treatMessages();
	};
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return replicas.isSynchronized();
	}));
	ASSERT_TRUE(replicas.requestObject(object));
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return !source.pending.empty();
	}));
	const auto [peer, request] = source.pending.front();
	client.disconnect();
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return connection && !source.peerID(*connection);
	}));
	EXPECT_FALSE(source.fulfillRequest(peer, request, payload(99)));
	EXPECT_FALSE(replicas.requestStatus(object));
	EXPECT_TRUE(replicas.objects.empty());
}

TEST_F(NetworkBindingTest, DuplicateHelloPreservesSessionAndUnbindingSourceStopsDelivery)
{
	Server server;
	Client client;
	Source source;
	Replicas replicas;
	Object entity;
	source.registerObject(object, entity);
	source.bind(server);
	replicas.bind(client);
	std::vector<spk::Message> hellos;
	auto observer = server.messageDispatcher().subscribeTo(42, [&](const auto &received) {
		if (Protocol(42).kind(received.message) == Protocol::Kind::Hello)
		{
			hellos.push_back(received.message);
		}
	});
	server.start(0);
	client.connect("127.0.0.1", server.port());
	auto pump = [&] {
		client.treatMessages();
		server.treatMessages();
		source.dispatch(Clock::now());
		client.treatMessages();
	};
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return replicas.isSynchronized();
	}));
	ASSERT_TRUE(replicas.requestObject(object));
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return replicas.objects.contains(object);
	}));
	client.send(hellos.back());
	const auto count = hellos.size();
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return hellos.size() > count;
	}));
	entity.change(8);
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		pump();
		return replicas.objects.at(object).value == 8;
	}));
	EXPECT_EQ(replicas.creations, 1);
	source.unbind();
	entity.change(10);
	EXPECT_EQ(source.dispatch(Clock::now()).sent, 0u);
	replicas.unbind();
	EXPECT_TRUE(replicas.objects.empty());
	EXPECT_THROW((void)replicas.requestObject(object), spk::Exception);
}
