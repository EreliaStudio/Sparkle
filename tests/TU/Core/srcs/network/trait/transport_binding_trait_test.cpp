#include "network/network_binding_test.hpp"

TEST_F(NetworkBindingTest, RawBindingTraitsExchangeMessagesAndTreatDisconnectWithoutReplication)
{
	class Echo final : public spk::Network::ServerBindingTrait
	{
		void _onServerMessage(const spk::ReceivedMessage &message) override
		{
			EXPECT_EQ(std::this_thread::get_id(), owner);
			EXPECT_TRUE(_sendTo(message.emitter, message.message));
		}
		void _onServerConnectionClosed(spk::ConnectionID) override
		{
			EXPECT_EQ(std::this_thread::get_id(), owner);
			++closed;
		}

	public:
		std::thread::id owner = std::this_thread::get_id();
		int closed = 0;
	} echo;
	class Receiver final : public spk::Network::ClientBindingTrait
	{
		void _onClientMessage(const spk::Message &message) override
		{
			EXPECT_EQ(std::this_thread::get_id(), owner);
			value = message.reader().get<int>();
		}
		void _onClientConnectionChanged() override
		{
			EXPECT_EQ(std::this_thread::get_id(), owner);
			++changes;
		}

	public:
		using ClientBindingTrait::_sendToServer;
		std::thread::id owner = std::this_thread::get_id();
		int value = 0, changes = 0;
	} receiver;
	Server server;
	Client client;
	echo.bind(server, 77);
	receiver.bind(client, 77);
	server.start(0);
	client.connect("127.0.0.1", server.port());
	spk::Message::Writer writer(77);
	writer << 123;
	ASSERT_TRUE(receiver._sendToServer(std::move(writer).build()));
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		server.treatMessages();
		client.treatMessages();
		return receiver.value == 123;
	}));
	const auto changes = receiver.changes;
	client.disconnect();
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		server.treatMessages();
		client.treatMessages();
		return echo.closed == 1 && receiver.changes > changes;
	}));
	echo.unbind();
	receiver.unbind();
}
