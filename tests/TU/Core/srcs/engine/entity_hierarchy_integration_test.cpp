#include "engine/client_entity_hierarchy.hpp"
#include "engine/publishable_entity3d.hpp"
#include "test_server_entity_hierarchy.hpp"
#include "network_test_utils.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace
{
	class Monster : public spk::PublishableEntity3D
	{
	public:
		explicit Monster(const std::string &name, spk::Entity *parent = nullptr) :
			PublishableEntity3D(Mode::Source, name, parent)
		{
		}

		explicit Monster(const spk::UUID &uuid) :
			PublishableEntity3D(Mode::Replicant, uuid, "Replicated monster")
		{
		}
	};

	constexpr spk::EntityHierarchy::MessageTypes Types{700001, 700002, 700003};

	bool pumpUntil(spk::Server &server, spk::Client &client, const std::function<bool()> &condition)
	{
		const auto deadline = std::chrono::steady_clock::now() + 5s;
		while (std::chrono::steady_clock::now() < deadline)
		{
			server.treatMessages();
			client.treatMessages();
			if (condition())
			{
				return true;
			}
			std::this_thread::sleep_for(5ms);
		}
		return condition();
	}
}

TEST(EntityHierarchyIntegrationTest, SelectedChildIncludesAncestorButNotSibling)
{
	spk::Server server;
	spk::Client client;
	spk::Entity root("server root");
	spk::Entity clientRoot("client root");
	Monster parent("parent", &root);
	Monster child("child", &parent);
	Monster sibling("sibling", &root);
	std::vector<spk::PublishableEntity *> selected{&child};

	TestServerEntityHierarchy serverHierarchy(
		server,
		root,
		[&](const spk::Connection &) {
			return selected;
		},
		Types);
	serverHierarchy.registerType<Monster>(42);
	spk::ClientEntityHierarchy clientHierarchy(client, clientRoot, Types);
	clientHierarchy.registerType(42, [](const spk::UUID &uuid) -> std::unique_ptr<spk::Entity> {
		return std::make_unique<Monster>(uuid);
	});

	server.start(0);
	(void)client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));
	EXPECT_EQ(server.connections().size(), 1u);

	auto result = serverHierarchy.publish(server.connections(), spk::Publishable::UpdateSelection::All);
	ASSERT_EQ(result.accepted.size(), 1u);
	ASSERT_TRUE(result.failures.empty());
	ASSERT_TRUE(pumpUntil(server, client, [&]() {
		return clientHierarchy.find(parent.uuid()) != nullptr && clientHierarchy.find(child.uuid()) != nullptr;
	}));
	EXPECT_EQ(clientHierarchy.size(), 2u);
	EXPECT_EQ(clientHierarchy.find(sibling.uuid()), nullptr);
	EXPECT_EQ(clientHierarchy.find(parent.uuid())->parent(), &clientRoot);
	EXPECT_EQ(clientHierarchy.find(child.uuid())->parent(), clientHierarchy.find(parent.uuid()));

	child.transform().place(spk::Vector3{3, 4, 5});
	result = serverHierarchy.publish(server.connections());
	ASSERT_TRUE(result.failures.empty());
	ASSERT_TRUE(pumpUntil(server, client, [&]() {
		auto *replica = dynamic_cast<Monster *>(clientHierarchy.find(child.uuid()));
		return replica != nullptr && replica->transform().position() == spk::Vector3{3, 4, 5};
	}));

	selected.clear();
	result = serverHierarchy.publish(server.connections());
	ASSERT_TRUE(result.failures.empty());
	ASSERT_TRUE(pumpUntil(server, client, [&]() {
		return clientHierarchy.size() == 0;
	}));
	EXPECT_TRUE(clientRoot.children().empty());
	client.disconnect();
	server.stop();
}

TEST(EntityHierarchyIntegrationTest, DifferentConnectionsReceiveIndependentSelections)
{
	spk::Server server;
	spk::Client firstClient;
	spk::Client secondClient;
	spk::Entity root("server root");
	spk::Entity firstRoot("first client root");
	spk::Entity secondRoot("second client root");
	Monster first("first", &root);
	Monster second("second", &root);

	spk::Connection::ID firstConnectionID{};
	TestServerEntityHierarchy serverHierarchy(
		server,
		root,
		[&](const spk::Connection &connection) {
			return connection.id() == firstConnectionID
				? std::vector<spk::PublishableEntity *>{&first}
				: std::vector<spk::PublishableEntity *>{&second};
		},
		Types);
	serverHierarchy.registerType<Monster>(42);
	spk::ClientEntityHierarchy firstHierarchy(firstClient, firstRoot, Types);
	spk::ClientEntityHierarchy secondHierarchy(secondClient, secondRoot, Types);
	firstHierarchy.registerType(42, [](const spk::UUID &uuid) -> std::unique_ptr<spk::Entity> {
		return std::make_unique<Monster>(uuid);
	});
	secondHierarchy.registerType(42, [](const spk::UUID &uuid) -> std::unique_ptr<spk::Entity> {
		return std::make_unique<Monster>(uuid);
	});

	server.start(0);
	(void)firstClient.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(firstClient));
	{
		const auto recipients = server.connections();
		ASSERT_EQ(recipients.size(), 1u);
		firstConnectionID = recipients.begin()->id();
	}
	(void)secondClient.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(secondClient));
	ASSERT_EQ(server.connections().size(), 2u);

	const auto result = serverHierarchy.publish(server.connections(), spk::Publishable::UpdateSelection::All);
	ASSERT_EQ(result.accepted.size(), 2u);
	ASSERT_TRUE(result.failures.empty());
	const auto deadline = std::chrono::steady_clock::now() + 5s;
	bool initialized = false;
	while (std::chrono::steady_clock::now() < deadline)
	{
		server.treatMessages();
		firstClient.treatMessages();
		secondClient.treatMessages();
		if (firstHierarchy.find(first.uuid()) != nullptr && secondHierarchy.find(second.uuid()) != nullptr)
		{
			initialized = true;
			break;
		}
		std::this_thread::sleep_for(5ms);
	}
	ASSERT_TRUE(initialized);
	EXPECT_EQ(firstHierarchy.find(second.uuid()), nullptr);
	EXPECT_EQ(secondHierarchy.find(first.uuid()), nullptr);
	EXPECT_EQ(firstHierarchy.size(), 1u);
	EXPECT_EQ(secondHierarchy.size(), 1u);

	firstClient.disconnect();
	secondClient.disconnect();
	server.stop();
}

TEST(EntityHierarchyIntegrationTest, ReparentingPreservesUUIDAndReconstructsHierarchy)
{
	spk::Server server;
	spk::Client client;
	spk::Entity root("server root");
	spk::Entity clientRoot("client root");
	Monster first("first", &root);
	Monster second("second", &root);
	Monster child("child", &first);

	TestServerEntityHierarchy serverHierarchy(
		server,
		root,
		[&](const spk::Connection &) {
			return std::vector<spk::PublishableEntity *>{&child};
		},
		Types);
	serverHierarchy.registerType<Monster>(42);
	spk::ClientEntityHierarchy clientHierarchy(client, clientRoot, Types);
	clientHierarchy.registerType(42, [](const spk::UUID &uuid) -> std::unique_ptr<spk::Entity> {
		return std::make_unique<Monster>(uuid);
	});

	server.start(0);
	(void)client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));
	(void)serverHierarchy.publish(server.connections());
	ASSERT_TRUE(pumpUntil(server, client, [&]() {
		return clientHierarchy.find(child.uuid()) != nullptr && clientHierarchy.find(first.uuid()) != nullptr;
	}));

	const spk::UUID persistent = child.uuid();
	child.setParent(second);
	(void)serverHierarchy.publish(server.connections(), spk::Publishable::UpdateSelection::All);
	ASSERT_TRUE(pumpUntil(server, client, [&]() {
		return clientHierarchy.find(second.uuid()) != nullptr &&
			   clientHierarchy.find(first.uuid()) == nullptr &&
			   clientHierarchy.find(child.uuid()) != nullptr &&
			   clientHierarchy.find(child.uuid())->parent() == clientHierarchy.find(second.uuid());
	}));
	EXPECT_EQ(child.uuid(), persistent);
	client.disconnect();
	server.stop();
}

TEST(EntityHierarchyProtocolTest, MalformedPublicationDoesNotChangeSelection)
{
	spk::Client client;
	spk::Entity root("replica root");
	spk::ClientEntityHierarchy hierarchy(client, root, Types);
	spk::Client::MessageQueue queue;

	spk::Message::Writer writer(Types.publication);
	writer << std::uint64_t{1} << std::uint32_t{1};
	queue.publish(std::move(writer).build());
	EXPECT_THROW(client.messageDispatcher().treatMessages(queue), spk::Exception);
	EXPECT_EQ(hierarchy.size(), 0u);
	EXPECT_TRUE(root.children().empty());
}

TEST(EntityHierarchyProtocolTest, DistinctMessageIdentifiersAreRequired)
{
	spk::Client client;
	spk::Entity root("replica root");
	EXPECT_THROW((void)spk::ClientEntityHierarchy(client, root, {7, 7, 8}), spk::Exception);
}

TEST(EntityHierarchyProtocolTest, InvalidRetryConfigurationIsRejected)
{
	spk::Client client;
	spk::Entity root("replica root");
	spk::ClientEntityHierarchy hierarchy(client, root, Types);
	EXPECT_THROW(hierarchy.setInitializationConfig({0, 1ms}), spk::Exception);
	EXPECT_THROW(hierarchy.setInitializationConfig({4, -1ms}), spk::Exception);
}
