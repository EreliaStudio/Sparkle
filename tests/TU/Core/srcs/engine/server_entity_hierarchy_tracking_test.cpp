#include "engine/publishable_entity3d.hpp"
#include "test_server_entity_hierarchy.hpp"
#include "exception.hpp"
#include "network/client.hpp"
#include "network_test_utils.hpp"

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
	spk::Entity root("root");
	TrackedSource *current = nullptr;
	TestServerEntityHierarchy hierarchy(
		server,
		root,
		[&](const spk::Connection &) {
			return current == nullptr ? std::vector<spk::PublishableEntity *>{} : std::vector<spk::PublishableEntity *>{current};
		},
		FirstTypes);
	hierarchy.registerType<TrackedSource>(7);
	server.start(0);
	(void)client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));
	(void)hierarchy.publish(server.connections());
	{
		TrackedSource transient(&root);
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
	spk::Entity firstRoot("first");
	spk::Entity secondRoot("second");
	TrackedSource source(&firstRoot);
	bool moved = false;
	TestServerEntityHierarchy first(
		server,
		firstRoot,
		[&](const spk::Connection &) {
			return moved ? std::vector<spk::PublishableEntity *>{} : std::vector<spk::PublishableEntity *>{&source};
		},
		FirstTypes);
	TestServerEntityHierarchy second(
		server,
		secondRoot,
		[&](const spk::Connection &) {
			return moved ? std::vector<spk::PublishableEntity *>{&source} : std::vector<spk::PublishableEntity *>{};
		},
		SecondTypes);
	first.registerType<TrackedSource>(7);
	second.registerType<TrackedSource>(7);
	server.start(0);
	(void)client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));

	EXPECT_NO_THROW((void)first.publish(server.connections()));
	EXPECT_NO_THROW((void)second.publish(server.connections()));
	const auto uuid = source.uuid();
	source.setParent(secondRoot);
	moved = true;
	EXPECT_NO_THROW((void)first.publish(server.connections()));
	EXPECT_NO_THROW((void)second.publish(server.connections()));
	EXPECT_EQ(source.uuid(), uuid);
	client.disconnect();
	server.stop();
}

TEST(ServerEntityHierarchyTrackingTest, OverlappingHierarchiesAreCallerResponsibility)
{
	spk::Server server;
	spk::Client client;
	spk::Entity root("root");
	TrackedSource intermediate(&root);
	TrackedSource child(&intermediate);
	TestServerEntityHierarchy outer(
		server,
		root,
		[&](const spk::Connection &) {
			return std::vector<spk::PublishableEntity *>{&child};
		},
		FirstTypes);
	TestServerEntityHierarchy inner(
		server,
		intermediate,
		[&](const spk::Connection &) {
			return std::vector<spk::PublishableEntity *>{&child};
		},
		SecondTypes);
	outer.registerType<TrackedSource>(7);
	inner.registerType<TrackedSource>(7);
	server.start(0);
	(void)client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));

	EXPECT_NO_THROW((void)outer.publish(server.connections(), spk::Publishable::UpdateSelection::All));
	EXPECT_NO_THROW((void)inner.publish(server.connections(), spk::Publishable::UpdateSelection::All));
	client.disconnect();
	server.stop();
}

TEST(EntityHierarchyStressTest, RepeatedSourceConstructionAndRemovalDuringPublications)
{
	spk::Server server;
	spk::Client client;
	spk::Entity root("root");
	TrackedSource *current = nullptr;
	TestServerEntityHierarchy hierarchy(
		server,
		root,
		[&](const spk::Connection &) {
			return current == nullptr ? std::vector<spk::PublishableEntity *>{} : std::vector<spk::PublishableEntity *>{current};
		},
		FirstTypes);
	hierarchy.registerType<TrackedSource>(7);
	server.start(0);
	(void)client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));
	(void)hierarchy.publish(server.connections());

	for (int index = 0; index < 250; ++index)
	{
		auto source = std::make_unique<TrackedSource>(&root);
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
	EXPECT_TRUE(root.children().empty());
	EXPECT_NO_THROW((void)hierarchy.publish(server.connections()));
	client.disconnect();
	server.stop();
}
