#include "engine/client_entity_hierarchy.hpp"
#include "engine/publishable_entity3d.hpp"
#include "network_test_utils.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace
{
	constexpr spk::EntityHierarchy::MessageTypes Types{710001, 710002, 710003};

	class Remote : public spk::PublishableEntity3D
	{
	public:
		Remote() :
			PublishableEntity3D(Mode::Source, "Source")
		{
		}

		explicit Remote(const spk::UUID &uuid) :
			PublishableEntity3D(Mode::Replicant, uuid, "Replicant")
		{
		}
	};

	spk::Message selection(std::uint64_t sequence, const std::vector<std::pair<spk::UUID, spk::UUID>> &members)
	{
		spk::Message::Writer writer(Types.publication);
		writer << sequence << static_cast<std::uint32_t>(members.size());
		for (const auto &[uuid, parent] : members)
		{
			writer << uuid.bytes() << parent.bytes() << std::uint64_t{0};
		}
		return std::move(writer).build();
	}

	spk::Message response(
		spk::Message::RequestID requestID,
		const spk::UUID &uuid,
		spk::EntityHierarchy::InitializationOutcome outcome,
		const spk::ByteStream &parts = {})
	{
		spk::Message::Writer writer(Types.initializationResponse);
		writer.setRequestID(requestID);
		writer << uuid.bytes() << static_cast<std::uint8_t>(outcome);
		if (outcome == spk::EntityHierarchy::InitializationOutcome::Success)
		{
			writer << std::uint32_t{42} << static_cast<std::uint64_t>(parts.size());
			writer.payload().append(parts.data().data(), parts.size());
		}
		return std::move(writer).build();
	}

	void deliver(spk::Client &client, spk::Message message)
	{
		spk::Client::MessageQueue messages;
		messages.publish(std::move(message));
		client.messageDispatcher().treatMessages(messages);
	}

	std::vector<spk::ReceivedMessage> requests(spk::Server &server)
	{
		std::vector<spk::ReceivedMessage> items;
		const auto deadline = std::chrono::steady_clock::now() + 5s;
		while (std::chrono::steady_clock::now() < deadline)
		{
			server.messages().drain(items);
			if (!items.empty())
			{
				return items;
			}
			std::this_thread::sleep_for(5ms);
		}
		return items;
	}
}

TEST(ClientEntityHierarchyTest, RetriesOnlyOnNewerPublicationsAndStopsAfterFourSends)
{
	spk::Server server;
	spk::Client client;
	spk::Entity root("root");
	spk::ClientEntityHierarchy hierarchy(client, root, Types);
	Remote source;
	server.start(0);
	(void)client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));

	for (std::uint64_t sequence = 1; sequence <= 4; ++sequence)
	{
		deliver(client, selection(sequence, {{source.uuid(), spk::UUID::null()}}));
		const auto sent = requests(server);
		ASSERT_EQ(sent.size(), 1u);
		EXPECT_EQ(sent[0].message.type(), Types.initializationRequest);
		EXPECT_EQ(sent[0].message.requestID(), sequence);
	}
	std::this_thread::sleep_for(30ms);
	std::vector<spk::ReceivedMessage> none;
	server.messages().drain(none);
	EXPECT_TRUE(none.empty());

	deliver(client, selection(5, {{source.uuid(), spk::UUID::null()}}));
	std::this_thread::sleep_for(50ms);
	server.messages().drain(none);
	EXPECT_TRUE(none.empty());

	deliver(client, selection(6, {}));
	deliver(client, selection(7, {{source.uuid(), spk::UUID::null()}}));
	EXPECT_EQ(requests(server).size(), 1u);
	client.disconnect();
	server.stop();
}

TEST(ClientEntityHierarchyTest, RejectedStopsRetriesUntilSelectionReentry)
{
	spk::Server server;
	spk::Client client;
	spk::Entity root("root");
	spk::ClientEntityHierarchy hierarchy(client, root, Types);
	Remote source;
	server.start(0);
	(void)client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));

	deliver(client, selection(1, {{source.uuid(), spk::UUID::null()}}));
	const auto sent = requests(server);
	ASSERT_EQ(sent.size(), 1u);
	deliver(client, response(sent[0].message.requestID(), source.uuid(), spk::EntityHierarchy::InitializationOutcome::Rejected));
	deliver(client, selection(2, {{source.uuid(), spk::UUID::null()}}));
	std::this_thread::sleep_for(40ms);
	std::vector<spk::ReceivedMessage> none;
	server.messages().drain(none);
	EXPECT_TRUE(none.empty());

	deliver(client, selection(3, {}));
	deliver(client, selection(4, {{source.uuid(), spk::UUID::null()}}));
	EXPECT_EQ(requests(server).size(), 1u);
	client.disconnect();
	server.stop();
}

TEST(ClientEntityHierarchyTest, LateInitializationPersistsUntilNextSelection)
{
	spk::Server server;
	spk::Client client;
	spk::Entity root("root");
	spk::ClientEntityHierarchy hierarchy(client, root, Types);
	hierarchy.registerType(42, [](const spk::UUID &uuid) -> std::unique_ptr<spk::Entity> {
		return std::make_unique<Remote>(uuid);
	});
	Remote source;
	server.start(0);
	(void)client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));

	deliver(client, selection(1, {{source.uuid(), spk::UUID::null()}}));
	const auto sent = requests(server);
	ASSERT_EQ(sent.size(), 1u);
	deliver(client, selection(2, {}));
	auto lateResponse = response(
		sent[0].message.requestID(),
		source.uuid(),
		spk::EntityHierarchy::InitializationOutcome::Success,
		source.encodeInitialization());
	deliver(client, std::move(lateResponse));
	EXPECT_NE(hierarchy.find(source.uuid()), nullptr);
	EXPECT_EQ(hierarchy.find(source.uuid())->parent(), nullptr);

	deliver(client, selection(3, {}));
	EXPECT_EQ(hierarchy.find(source.uuid()), nullptr);
	client.disconnect();
	server.stop();
}

TEST(ClientEntityHierarchyTest, ChildInitializesDetachedAndAttachesWhenParentArrives)
{
	spk::Server server;
	spk::Client client;
	spk::Entity root("root");
	spk::ClientEntityHierarchy hierarchy(client, root, Types);
	hierarchy.registerType(42, [](const spk::UUID &uuid) -> std::unique_ptr<spk::Entity> {
		return std::make_unique<Remote>(uuid);
	});
	Remote parent;
	Remote child;
	server.start(0);
	(void)client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));

	deliver(client, selection(1, {{parent.uuid(), spk::UUID::null()}, {child.uuid(), parent.uuid()}}));
	const auto sent = requests(server);
	ASSERT_EQ(sent.size(), 1u);
	auto requestID = sent[0].message.requestID();
	deliver(client, response(requestID, child.uuid(), spk::EntityHierarchy::InitializationOutcome::Success, child.encodeInitialization()));
	ASSERT_NE(hierarchy.find(child.uuid()), nullptr);
	EXPECT_EQ(hierarchy.find(child.uuid())->parent(), nullptr);

	deliver(client, response(requestID, parent.uuid(), spk::EntityHierarchy::InitializationOutcome::Success, parent.encodeInitialization()));
	ASSERT_NE(hierarchy.find(parent.uuid()), nullptr);
	EXPECT_EQ(hierarchy.find(parent.uuid())->parent(), &root);
	EXPECT_EQ(hierarchy.find(child.uuid())->parent(), hierarchy.find(parent.uuid()));
	client.disconnect();
	server.stop();
}

TEST(ClientEntityHierarchyTest, IntervalPreventsRapidRetriesButNoTimerSendsByItself)
{
	spk::Server server;
	spk::Client client;
	spk::Entity root("root");
	spk::ClientEntityHierarchy hierarchy(client, root, Types, {4, 150ms});
	Remote source;
	server.start(0);
	(void)client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));

	deliver(client, selection(1, {{source.uuid(), spk::UUID::null()}}));
	EXPECT_EQ(requests(server).size(), 1u);
	deliver(client, selection(2, {{source.uuid(), spk::UUID::null()}}));
	std::this_thread::sleep_for(200ms);
	std::vector<spk::ReceivedMessage> none;
	server.messages().drain(none);
	EXPECT_TRUE(none.empty());
	deliver(client, selection(3, {{source.uuid(), spk::UUID::null()}}));
	EXPECT_EQ(requests(server).size(), 1u);
	client.disconnect();
	server.stop();
}
