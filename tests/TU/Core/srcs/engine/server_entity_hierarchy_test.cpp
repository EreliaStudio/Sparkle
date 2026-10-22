#include "engine/publishable_entity3d.hpp"
#include "engine/server_entity_hierarchy.hpp"
#include "exception.hpp"
#include "network/client.hpp"
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
	spk::ServerEntityHierarchy hierarchy(
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
	spk::ServerEntityHierarchy hierarchy(
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
