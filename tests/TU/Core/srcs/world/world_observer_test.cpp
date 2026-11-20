#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <set>
#include <type_traits>
#include <vector>

#include "engine/component3d.hpp"
#include "engine/engine_headless.hpp"
#include "engine/entity3d.hpp"
#include "engine/registry.hpp"
#include "engine/transform3d.hpp"
#include "exception.hpp"
#include "world/chunk.hpp"
#include "world/chunk_interest.hpp"
#include "world/world_observer.hpp"

namespace
{
	using Coordinate = spk::ChunkCoordinate;
	using ObserverRegistry = spk::Registry<spk::WorldObserver, spk::EngineHeadless *>;
	using TestChunk = spk::Chunk<16, 8, 32>;

	static_assert(std::derived_from<spk::WorldObserver, spk::Component3D>);

	[[nodiscard]] Coordinate centerOf(const spk::WorldObserver &observer)
	{
		return TestChunk::worldToChunkCoordinate(observer.worldPosition());
	}
}

TEST(ChunkInterestTest, RejectsNegativeRadiusOnEachAxis)
{
	EXPECT_THROW((void)spk::ChunkInterest({-1, 0, 0}), spk::Exception);
	EXPECT_THROW((void)spk::ChunkInterest({0, -1, 0}), spk::Exception);
	EXPECT_THROW((void)spk::ChunkInterest({0, 0, -1}), spk::Exception);
	EXPECT_EQ(spk::ChunkInterest().radius(), (spk::Vector3Int{0, 0, 0}));
}

TEST(ChunkInterestTest, RadiusIsInclusiveAndHasFourHundredAndFiveCoordinates)
{
	const spk::ChunkInterest interest({4, 2, 4});
	const Coordinate center{-3, 5, -7};
	const std::vector<Coordinate> coords = interest.coordinates(center);
	const std::set<Coordinate> unique(coords.begin(), coords.end());

	EXPECT_EQ(interest.radius(), (spk::Vector3Int{4, 2, 4}));
	ASSERT_EQ(coords.size(), 405u);
	EXPECT_EQ(unique.size(), 405u);
	EXPECT_TRUE(unique.contains(center));
	EXPECT_TRUE(unique.contains({-7, 3, -11}));
	EXPECT_TRUE(unique.contains({1, 7, -3}));
	EXPECT_FALSE(unique.contains({-8, 5, -7}));
	EXPECT_FALSE(unique.contains({-3, 8, -7}));
	EXPECT_FALSE(unique.contains({-3, 5, -2}));
}

TEST(ChunkInterestTest, ZeroRadiusRequestsOnlyItsCenter)
{
	const Coordinate center{13, -7, 2};
	const std::vector<Coordinate> coords = spk::ChunkInterest().coordinates(center);

	ASSERT_EQ(coords.size(), 1u);
	EXPECT_EQ(coords.front(), center);
}

TEST(ChunkInterestTest, BoundaryOverflowThrowsInsteadOfWrappingCoordinates)
{
	const spk::ChunkInterest interest({1, 1, 1});
	EXPECT_THROW((void)interest.coordinates({std::numeric_limits<std::int32_t>::max(), 0, 0}), spk::Exception);
	EXPECT_THROW((void)interest.coordinates({0, std::numeric_limits<std::int32_t>::min(), 0}), spk::Exception);
	EXPECT_THROW((void)interest.coordinates({0, 0, std::numeric_limits<std::int32_t>::max()}), spk::Exception);

	const Coordinate nearLimit{std::numeric_limits<std::int32_t>::max() - 1, 0, 0};
	EXPECT_EQ(spk::ChunkInterest({1, 0, 0}).coordinates(nearLimit).size(), 3u);
}

TEST(WorldObserverTest, DetachedObserverDoesNotHaveAWorldPosition)
{
	spk::WorldObserver observer;
	EXPECT_EQ(observer.context(), nullptr);
	EXPECT_EQ(observer.owner(), nullptr);
	EXPECT_THROW((void)observer.worldPosition(), spk::Exception);
}

TEST(WorldObserverTest, RegistryTracksOnlyObserverComponentsInTheEngine)
{
	spk::EngineHeadless engine;
	spk::Entity3D player("player");
	spk::Entity3D spectator("spectator");
	auto &observer = player.addComponent<spk::WorldObserver>(spk::ChunkInterest({2, 1, 3}));
	engine.addEntity(&player);
	engine.addEntity(&spectator);

	auto &registry = ObserverRegistry::instance();
	EXPECT_EQ(observer.owner(), &player);
	EXPECT_TRUE(registry.elements(&engine).contains(&observer));
	EXPECT_EQ(registry.elements(&engine).size(), 1u);
	EXPECT_EQ(spectator.getComponent<spk::WorldObserver>(), nullptr);

	engine.removeEntity(&player);
	EXPECT_FALSE(registry.elements(&engine).contains(&observer));
	EXPECT_TRUE(registry.elements(nullptr).contains(&observer));
	EXPECT_EQ(observer.owner(), &player);

	engine.addEntity(&player);
	EXPECT_TRUE(registry.elements(&engine).contains(&observer));
}

TEST(WorldObserverTest, AdditionRemovalAndRebindingAreObservedByTheExistingRegistry)
{
	spk::EngineHeadless first;
	spk::EngineHeadless second;
	spk::Entity3D player("player");
	auto &registry = ObserverRegistry::instance();
	int firstAdditions = 0;
	int firstRemovals = 0;
	int secondAdditions = 0;
	auto onFirstAdd = registry.subscribeToAddition(&first, [&](spk::EngineHeadless *const &, spk::WorldObserver *) {
		++firstAdditions;
	});
	auto onFirstRemove = registry.subscribeToRemoval(&first, [&](spk::EngineHeadless *const &, spk::WorldObserver *) {
		++firstRemovals;
	});
	auto onSecondAdd = registry.subscribeToAddition(&second, [&](spk::EngineHeadless *const &, spk::WorldObserver *) {
		++secondAdditions;
	});

	first.addEntity(&player);
	auto &observer = player.addComponent<spk::WorldObserver>();
	EXPECT_EQ(firstAdditions, 1);
	EXPECT_TRUE(registry.elements(&first).contains(&observer));

	second.addEntity(&player);
	EXPECT_EQ(firstRemovals, 1);
	EXPECT_EQ(secondAdditions, 1);
	EXPECT_FALSE(registry.elements(&first).contains(&observer));
	EXPECT_TRUE(registry.elements(&second).contains(&observer));

	player.removeComponent(observer);
	EXPECT_FALSE(registry.elements(&second).contains(&observer));
	EXPECT_TRUE(registry.elements(&second).empty());
}

TEST(WorldObserverTest, MultipleObserversAreTrackedIndependently)
{
	spk::EngineHeadless engine;
	spk::Entity3D firstPlayer("first");
	spk::Entity3D secondPlayer("second");
	auto &first = firstPlayer.addComponent<spk::WorldObserver>(spk::ChunkInterest({1, 2, 3}));
	auto &second = secondPlayer.addComponent<spk::WorldObserver>(spk::ChunkInterest({4, 0, 2}));

	engine.addEntity(&firstPlayer);
	engine.addEntity(&secondPlayer);
	const auto &registered = ObserverRegistry::instance().elements(&engine);
	ASSERT_EQ(registered.size(), 2u);
	EXPECT_TRUE(registered.contains(&first));
	EXPECT_TRUE(registered.contains(&second));

	engine.removeEntity(&firstPlayer);
	EXPECT_FALSE(registered.contains(&first));
	EXPECT_TRUE(registered.contains(&second));
	EXPECT_EQ(registered.size(), 1u);
}

TEST(WorldObserverTest, WorldPositionIncludesParentTransformsAndReparenting)
{
	spk::Entity3D originalParent("original");
	spk::Entity3D otherParent("other");
	spk::Entity3D child("child", &originalParent);
	auto &observer = child.addComponent<spk::WorldObserver>();
	originalParent.transform().place({20.0f, 15.0f, 8.0f});
	child.transform().place({3.0f, 1.0f, -5.0f});
	EXPECT_EQ(observer.worldPosition(), (spk::Vector3{23.0f, 16.0f, 3.0f}));

	otherParent.transform().place({-10.0f, 0.0f, 10.0f});
	child.setParent(otherParent);
	EXPECT_EQ(observer.worldPosition(), (spk::Vector3{-7.0f, 1.0f, 5.0f}));
}

TEST(WorldObserverTest, IntraChunkMovementPreservesCenterWhileBoundaryCrossingChangesIt)
{
	spk::Entity3D entity("observer");
	auto &observer = entity.addComponent<spk::WorldObserver>(spk::ChunkInterest({1, 1, 1}));
	entity.transform().place({-0.5f, 7.0f, -0.5f});

	const Coordinate initial = centerOf(observer);
	EXPECT_EQ(initial, (Coordinate{-1, 0, -1}));

	entity.transform().place({-15.5f, 0.1f, -31.5f});
	EXPECT_EQ(centerOf(observer), initial);
	EXPECT_EQ(observer.interest().coordinates(centerOf(observer)).size(), 27u);

	entity.transform().place({-16.1f, 8.1f, -32.1f});
	EXPECT_EQ(centerOf(observer), (Coordinate{-2, 1, -2}));
	EXPECT_NE(centerOf(observer), initial);
}

TEST(WorldObserverTest, InterestChangesNotifyOnceAndDoNotNotifyForEquivalentValues)
{
	spk::Entity3D entity("observer");
	auto &observer = entity.addComponent<spk::WorldObserver>();
	int calls = 0;
	auto contract = observer.subscribeToInterestEdition([&](const spk::WorldObserver &changed) {
		EXPECT_EQ(&changed, &observer);
		++calls;
	});

	observer.setInterest(spk::ChunkInterest({0, 0, 0}));
	EXPECT_EQ(calls, 0);
	observer.setInterest(spk::ChunkInterest({4, 2, 4}));
	EXPECT_EQ(calls, 1);
	EXPECT_EQ(observer.interest().coordinates({0, 0, 0}).size(), 405u);
	observer.setInterest(spk::ChunkInterest({4, 2, 4}));
	EXPECT_EQ(calls, 1);
	observer.setInterest(spk::ChunkInterest({1, 0, 1}));
	EXPECT_EQ(calls, 2);
	EXPECT_TRUE(contract.isValid());

	EXPECT_THROW((void)spk::ChunkInterest({-1, 0, 0}), spk::Exception);
	EXPECT_EQ(observer.interest().radius(), (spk::Vector3Int{1, 0, 1}));
	EXPECT_EQ(calls, 2);
}
