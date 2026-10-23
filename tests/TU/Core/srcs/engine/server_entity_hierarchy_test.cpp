#include "engine/publishable_entity3d.hpp"
#include "exception.hpp"
#include "network/client.hpp"
#include "network_test_utils.hpp"
#include "test_server_entity_hierarchy.hpp"

#include <gtest/gtest.h>

#include <algorithm>
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
	constexpr spk::EntityHierarchy::MessageConfiguration Types{{720001, spk::Message::Priority::Low}, {720002, spk::Message::Priority::High}, {720003, spk::Message::Priority::High}};

	class FailablePart : public spk::Publishable::Part
	{
	public:
		bool fail = false;
		int value = 13;
		mutable int encodes = 0;

		void edit(int next)
		{
			value = next;
			invalidate();
		}

	protected:
		void _encode(spk::ByteStream::Writer &writer) const override
		{
			if (fail)
			{
				throw spk::Exception("Intentional publication encoding failure.");
			}
			++encodes;
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
	Source source(nullptr);
	FailablePart part;
	source.setPartRegistry({{0, part, spk::Publishable::Publication::Update}});
	TestServerEntityHierarchy hierarchy(
		[&](const spk::Connection &) {
			return std::vector<spk::PublishableEntity *>{&source};
		},
		Types);
	hierarchy.bind(server);
	hierarchy.registerType<Source>(42);

	source.setParent(hierarchy.root());
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
	EXPECT_EQ(results.size(), 2u);
	std::size_t failures = 0;
	for (const auto &[connectionID, outcome] : results)
	{
		if (!outcome)
		{
			++failures;
		}
	}
	EXPECT_EQ(failures, 1u);
	EXPECT_TRUE(source.captureUpdate().bytes().empty());
	client.disconnect();
	server.stop();
}

TEST(ServerEntityHierarchyTest, InitializationAuthorizationDoesNotRevealUnknownEntity)
{
	spk::Server server;
	spk::Client client;
	TestServerEntityHierarchy hierarchy(
		[](const spk::Connection &) {
			return std::vector<spk::PublishableEntity *>{};
		},
		Types);
	hierarchy.bind(server);
	server.start(0);
	(void)client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));

	const spk::UUID unknown = spk::UUID::generate();
	spk::Message::Writer request(Types.initializationRequest.type);
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
	Source parent(nullptr);
	Source child(&parent);
	Source sibling(nullptr);
	std::vector<spk::PublishableEntity *> selected{&child};
	int selectionCalls = 0;
	TestServerEntityHierarchy hierarchy(
		[&](const spk::Connection &) {
			++selectionCalls;
			return selected;
		},
		Types);
	hierarchy.bind(server);
	hierarchy.registerType<Source>(42);

	parent.setParent(hierarchy.root());
	sibling.setParent(hierarchy.root());
	server.start(0);
	(void)client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));

	const spk::UUID unknown = spk::UUID::generate();
	auto request = [&](std::uint32_t requestID, const std::vector<spk::UUID> &ids) {
		spk::Message::Writer writer(Types.initializationRequest.type);
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
	spk::Entity nonPublishable("boundary");
	Source child(&nonPublishable);
	TestServerEntityHierarchy hierarchy(
		[&](const spk::Connection &) {
			return std::vector<spk::PublishableEntity *>{&child};
		},
		Types);
	hierarchy.bind(server);
	nonPublishable.setParent(hierarchy.root());
	server.start(0);
	(void)client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));

	EXPECT_THROW((void)hierarchy.publish(server.connections()), spk::Exception);
	hierarchy.registerType<Source>(42);
	EXPECT_THROW((void)hierarchy.publish(server.connections()), spk::Exception);
	child.setParent(hierarchy.root());
	EXPECT_NO_THROW((void)hierarchy.publish(server.connections()));

	client.disconnect();
	server.stop();
}

TEST(ServerEntityHierarchyTest, MultipleRecipientsCaptureOnceAndUnselectedChangesRemainPending)
{
	spk::Server server;
	spk::Client firstClient;
	spk::Client secondClient;
	Source first(nullptr);
	Source unselected(nullptr);
	FailablePart firstPart;
	FailablePart unselectedPart;
	first.setPartRegistry({{0, firstPart, spk::Publishable::Publication::Update}});
	unselected.setPartRegistry({{0, unselectedPart, spk::Publishable::Publication::Update}});
	std::vector<spk::PublishableEntity *> selected{&first};
	TestServerEntityHierarchy hierarchy(
		[&](const spk::Connection &) {
			return selected;
		},
		Types);
	hierarchy.bind(server);
	hierarchy.registerType<Source>(42);

	first.setParent(hierarchy.root());
	unselected.setParent(hierarchy.root());
	server.start(0);
	(void)firstClient.connect("127.0.0.1", server.port());
	(void)secondClient.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(firstClient));
	ASSERT_TRUE(NetworkTestUtils::waitConnected(secondClient));
	const auto recipients = server.connections();
	ASSERT_EQ(recipients.size(), 2u);

	firstPart.edit(25);
	unselectedPart.edit(30);
	const auto firstResult = hierarchy.publish(recipients);
	EXPECT_EQ(firstResult.size(), 2u);
	EXPECT_TRUE(std::ranges::all_of(firstResult, [](const auto &entry) {
		return entry.second.has_value();
	}));
	EXPECT_EQ(firstPart.encodes, 1);
	EXPECT_EQ(unselectedPart.encodes, 0);
	EXPECT_FALSE(unselected.captureUpdate().bytes().empty());

	selected = {&unselected};
	const auto secondResult = hierarchy.publish(recipients);
	EXPECT_EQ(secondResult.size(), 2u);
	EXPECT_TRUE(std::ranges::all_of(secondResult, [](const auto &entry) {
		return entry.second.has_value();
	}));
	EXPECT_EQ(unselectedPart.encodes, 1);
	EXPECT_TRUE(unselected.captureUpdate().bytes().empty());

	firstClient.disconnect();
	secondClient.disconnect();
	server.stop();
}

TEST(ServerEntityHierarchyTest, ConstructsBeforeServerAndBindsExplicitly)
{
	Source *selected = nullptr;
	TestServerEntityHierarchy hierarchy(
		[&](const spk::Connection &) {
			return selected == nullptr ? std::vector<spk::PublishableEntity *>{} : std::vector<spk::PublishableEntity *>{selected};
		},
		Types);
	EXPECT_THROW((void)hierarchy.publish({}), spk::Exception);

	spk::Server server;
	hierarchy.bind(server);
	EXPECT_THROW(hierarchy.bind(server), spk::Exception);
	Source source(&hierarchy.root());
	selected = &source;
	hierarchy.registerType<Source>(42);
	EXPECT_EQ(source.parent(), &hierarchy.root());
	EXPECT_NO_THROW((void)hierarchy.publish({}));
	hierarchy.clear(spk::EntityHierarchy::ClearMode::Full);
	EXPECT_NO_THROW(hierarchy.bind(server));
}
