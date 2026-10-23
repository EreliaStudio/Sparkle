#include "engine/publishable_entity3d.hpp"
#include "test_server_entity_hierarchy.hpp"
#include "exception.hpp"
#include "network/client.hpp"
#include "network_test_utils.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace
{
	constexpr spk::EntityHierarchy::MessageTypes Types{720001, 720002, 720003};

	class FailablePart : public spk::Publishable::Part
	{
	public:
		bool fail = false;
		int value = 13;

	protected:
		void _encode(spk::ByteStream::Writer &writer) const override
		{
			if (fail)
			{
				throw spk::Exception("Intentional publication encoding failure.");
			}
			writer << value;
		}

		void _decode(const spk::ByteStream::Reader &reader) override
		{
			reader >> value;
		}
	};

	class Source : public spk::PublishableEntity3D
	{
	public:
		Source(spk::Entity *parent) :
			PublishableEntity3D(Mode::Source, "Source", parent)
		{
		}

		using PublishableEntity::setPartRegistry;
	};
}

TEST(ServerEntityHierarchyTest, FailedPreparationPreservesModifiedAndTransportFailuresAreIndependent)
{
	spk::Server server;
	spk::Client client;
	spk::Entity root("root");
	Source source(&root);
	FailablePart part;
	source.setPartRegistry({{0, part, spk::Publishable::Publication::Update}});
	TestServerEntityHierarchy hierarchy(
		server,
		root,
		[&](const spk::Connection &) {
			return std::vector<spk::PublishableEntity *>{&source};
		},
		Types);
	hierarchy.registerType<Source>(42);

	server.start(0);
	(void)client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));

	part.fail = true;
	EXPECT_THROW((void)hierarchy.publish(server.connections()), spk::Exception);
	part.fail = false;
	EXPECT_FALSE(source.captureUpdate().bytes().empty());

	spk::ConnectionSet recipients = server.connections();
	recipients.insert(spk::Connection{});
	auto results = hierarchy.publish(recipients);
	EXPECT_EQ(results.accepted.size(), 1u);
	EXPECT_EQ(results.failures.size(), 1u);
	EXPECT_TRUE(source.captureUpdate().bytes().empty());
	client.disconnect();
	server.stop();
}

TEST(ServerEntityHierarchyTest, InitializationAuthorizationDoesNotRevealUnknownEntity)
{
	spk::Server server;
	spk::Client client;
	spk::Entity root("root");
	TestServerEntityHierarchy hierarchy(
		server,
		root,
		[](const spk::Connection &) {
			return std::vector<spk::PublishableEntity *>{};
		},
		Types);
	server.start(0);
	(void)client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));

	const spk::UUID unknown = spk::UUID::generate();
	spk::Message::Writer request(Types.initializationRequest);
	request.setRequestID(88);
	request << std::uint32_t{1} << unknown.bytes();
	(void)client.send(std::move(request).build());
	const auto deadline = std::chrono::steady_clock::now() + 5s;
	while (std::chrono::steady_clock::now() < deadline)
	{
		server.treatMessages();
		std::vector<spk::Message> messages;
		client.messages().drain(messages);
		if (!messages.empty())
		{
			ASSERT_EQ(messages.size(), 1u);
			EXPECT_EQ(messages[0].requestID(), 88u);
			const auto reader = messages[0].reader();
			spk::UUID::Storage bytes;
			std::uint8_t outcome;
			reader >> bytes >> outcome;
			EXPECT_EQ(spk::UUID(bytes), unknown);
			EXPECT_EQ(outcome, static_cast<std::uint8_t>(spk::EntityHierarchy::InitializationOutcome::Rejected));
			EXPECT_EQ(reader.remaining(), 0u);
			client.disconnect();
			server.stop();
			return;
		}
		std::this_thread::sleep_for(5ms);
	}
	FAIL() << "Did not receive initialization rejection.";
	client.disconnect();
	server.stop();
}

TEST(ServerEntityHierarchyTest, InitializationAuthorizesCurrentDirectAndAncestorSelectionOncePerBatch)
{
	spk::Server server;
	spk::Client client;
	spk::Entity root("root");
	Source parent(&root);
	Source child(&parent);
	Source sibling(&root);
	std::vector<spk::PublishableEntity *> selected{&child};
	int selectionCalls = 0;
	TestServerEntityHierarchy hierarchy(
		server,
		root,
		[&](const spk::Connection &) {
			++selectionCalls;
			return selected;
		},
		Types);
	hierarchy.registerType<Source>(42);

	server.start(0);
	(void)client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));

	const spk::UUID unknown = spk::UUID::generate();
	auto request = [&](std::uint32_t requestID, const std::vector<spk::UUID> &ids) {
		spk::Message::Writer writer(Types.initializationRequest);
		writer.setRequestID(requestID);
		writer << static_cast<std::uint32_t>(ids.size());
		for (const auto &uuid : ids)
		{
			writer << uuid.bytes();
		}
		(void)client.send(std::move(writer).build());

		std::map<spk::UUID, spk::EntityHierarchy::InitializationOutcome> outcomes;
		const auto deadline = std::chrono::steady_clock::now() + 5s;
		while (std::chrono::steady_clock::now() < deadline && outcomes.size() < ids.size())
		{
			server.treatMessages();
			std::vector<spk::Message> responses;
			client.messages().drain(responses);
			for (const auto &response : responses)
			{
				EXPECT_EQ(response.requestID(), requestID);
				const auto reader = response.reader();
				spk::UUID::Storage bytes;
				std::uint8_t rawOutcome;
				reader >> bytes >> rawOutcome;
				const auto outcome = static_cast<spk::EntityHierarchy::InitializationOutcome>(rawOutcome);
				if (outcome == spk::EntityHierarchy::InitializationOutcome::Success)
				{
					std::uint32_t type;
					std::uint64_t length;
					reader >> type >> length;
					EXPECT_EQ(type, 42u);
					EXPECT_GT(length, 0u);
					EXPECT_EQ(reader.remaining(), length);
					reader.skip(static_cast<std::size_t>(length));
				}
				EXPECT_EQ(reader.remaining(), 0u);
				outcomes.emplace(spk::UUID(bytes), outcome);
			}
			std::this_thread::sleep_for(5ms);
		}
		return outcomes;
	};

	using Outcome = spk::EntityHierarchy::InitializationOutcome;
	auto first = request(90, {parent.uuid(), child.uuid(), sibling.uuid(), unknown});
	ASSERT_EQ(first.size(), 4u);
	EXPECT_EQ(first.at(parent.uuid()), Outcome::Success);
	EXPECT_EQ(first.at(child.uuid()), Outcome::Success);
	EXPECT_EQ(first.at(sibling.uuid()), Outcome::Rejected);
	EXPECT_EQ(first.at(unknown), Outcome::Rejected);
	EXPECT_EQ(selectionCalls, 1);

	selected.clear();
	auto second = request(91, {parent.uuid(), child.uuid()});
	ASSERT_EQ(second.size(), 2u);
	EXPECT_EQ(second.at(parent.uuid()), Outcome::Rejected);
	EXPECT_EQ(second.at(child.uuid()), Outcome::Rejected);
	EXPECT_EQ(selectionCalls, 2);

	client.disconnect();
	server.stop();
}

TEST(ServerEntityHierarchyTest, InvalidAncestorsAndUnregisteredTypesRejectPublication)
{
	spk::Server server;
	spk::Client client;
	spk::Entity root("root");
	spk::Entity nonPublishable("boundary", &root);
	Source child(&nonPublishable);
	TestServerEntityHierarchy hierarchy(
		server,
		root,
		[&](const spk::Connection &) {
			return std::vector<spk::PublishableEntity *>{&child};
		},
		Types);
	server.start(0);
	(void)client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));

	EXPECT_THROW((void)hierarchy.publish(server.connections()), spk::Exception);
	hierarchy.registerType<Source>(42);
	EXPECT_THROW((void)hierarchy.publish(server.connections()), spk::Exception);
	child.setParent(root);
	EXPECT_NO_THROW((void)hierarchy.publish(server.connections()));

	client.disconnect();
	server.stop();
}
