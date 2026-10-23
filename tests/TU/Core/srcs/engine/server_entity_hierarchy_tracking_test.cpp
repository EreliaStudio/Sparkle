#include "engine/publishable_entity3d.hpp"
#include "exception.hpp"
#include "network/client.hpp"
#include "network_test_utils.hpp"
#include "test_server_entity_hierarchy.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace
{
	constexpr spk::EntityHierarchy::MessageTypes FirstTypes{730001, 730002, 730003};
	constexpr spk::EntityHierarchy::MessageTypes SecondTypes{730011, 730012, 730013};

	class TrackedSource : public spk::PublishableEntity3D
	{
	public:
		explicit TrackedSource(spk::Entity *parent) :
			PublishableEntity3D(Mode::Source, "Source", parent)
		{
		}
	};
}

TEST(EntityHierarchyMemorySafetyTest, ConstructedAndDestroyedBetweenCyclesIsNeverDereferenced)
{
	spk::Server server;
	spk::Client client;
	TrackedSource *current = nullptr;
	TestServerEntityHierarchy hierarchy(
		[&](const spk::Connection &) {
			return current == nullptr ? std::vector<spk::PublishableEntity *>{} : std::vector<spk::PublishableEntity *>{current};
		},
		FirstTypes);
	hierarchy.bind(server);
	hierarchy.registerType<TrackedSource>(7);
	server.start(0);
	(void)client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));
	(void)hierarchy.publish(server.connections());
	{
		TrackedSource transient(&hierarchy.root());
		current = &transient;
		EXPECT_NO_THROW((void)hierarchy.publish(server.connections()));
	}
	current = nullptr;
	EXPECT_NO_THROW((void)hierarchy.publish(server.connections()));
	client.disconnect();
	server.stop();
}

TEST(ServerEntityHierarchyTrackingTest, SourceMigratesBetweenHierarchyRootsWithoutChangingUUID)
{
	spk::Server server;
	spk::Client client;
	TrackedSource source(nullptr);
	bool moved = false;
	TestServerEntityHierarchy first(
		[&](const spk::Connection &) {
			return moved ? std::vector<spk::PublishableEntity *>{} : std::vector<spk::PublishableEntity *>{&source};
		},
		FirstTypes);
	TestServerEntityHierarchy second(
		[&](const spk::Connection &) {
			return moved ? std::vector<spk::PublishableEntity *>{&source} : std::vector<spk::PublishableEntity *>{};
		},
		SecondTypes);
	first.bind(server);
	second.bind(server);
	source.setParent(first.root());
	first.registerType<TrackedSource>(7);
	second.registerType<TrackedSource>(7);
	server.start(0);
	(void)client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));

	EXPECT_NO_THROW((void)first.publish(server.connections()));
	EXPECT_NO_THROW((void)second.publish(server.connections()));
	const auto uuid = source.uuid();
	source.setParent(second.root());
	moved = true;
	EXPECT_NO_THROW((void)first.publish(server.connections()));
	EXPECT_NO_THROW((void)second.publish(server.connections()));
	EXPECT_EQ(source.uuid(), uuid);
	client.disconnect();
	server.stop();
}

TEST(ServerEntityHierarchyTrackingTest, IndependentHierarchiesKeepSeparateOwnedRoots)
{
	spk::Server server;
	spk::Client client;
	TrackedSource outerSource(nullptr);
	TrackedSource innerSource(nullptr);
	TestServerEntityHierarchy outer(
		[&](const spk::Connection &) {
			return std::vector<spk::PublishableEntity *>{&outerSource};
		},
		FirstTypes);
	TestServerEntityHierarchy inner(
		[&](const spk::Connection &) {
			return std::vector<spk::PublishableEntity *>{&innerSource};
		},
		SecondTypes);
	outerSource.setParent(outer.root());
	innerSource.setParent(inner.root());
	outer.bind(server);
	inner.bind(server);
	outer.registerType<TrackedSource>(7);
	inner.registerType<TrackedSource>(7);
	server.start(0);
	(void)client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));

	EXPECT_NE(&outer.root(), &inner.root());
	EXPECT_EQ(outerSource.parent(), &outer.root());
	EXPECT_EQ(innerSource.parent(), &inner.root());
	EXPECT_NO_THROW((void)outer.publish(server.connections(), spk::Publishable::UpdateSelection::All));
	EXPECT_NO_THROW((void)inner.publish(server.connections(), spk::Publishable::UpdateSelection::All));
	client.disconnect();
	server.stop();
}

TEST(EntityHierarchyStressTest, RepeatedSourceConstructionAndRemovalDuringPublications)
{
	spk::Server server;
	spk::Client client;
	TrackedSource *current = nullptr;
	TestServerEntityHierarchy hierarchy(
		[&](const spk::Connection &) {
			return current == nullptr ? std::vector<spk::PublishableEntity *>{} : std::vector<spk::PublishableEntity *>{current};
		},
		FirstTypes);
	hierarchy.bind(server);
	hierarchy.registerType<TrackedSource>(7);
	server.start(0);
	(void)client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));
	(void)hierarchy.publish(server.connections());

	for (int index = 0; index < 250; ++index)
	{
		auto source = std::make_unique<TrackedSource>(&hierarchy.root());
		current = source.get();
		if (index % 2 == 0)
		{
			(void)hierarchy.publish(server.connections());
		}
		source.reset();
		current = nullptr;
		if (index % 4 == 0)
		{
			(void)hierarchy.publish(server.connections());
		}
	}
	EXPECT_TRUE(hierarchy.root().children().empty());
	EXPECT_NO_THROW((void)hierarchy.publish(server.connections()));
	client.disconnect();
	server.stop();
}
