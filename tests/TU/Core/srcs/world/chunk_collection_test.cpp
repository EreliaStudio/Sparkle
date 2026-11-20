#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "engine/engine_headless.hpp"
#include "engine/registry.hpp"
#include "engine/transform3d.hpp"
#include "world/chunk_collection.hpp"

namespace
{
	using Coordinate = spk::ChunkCoordinate;

	class TestChunk : public spk::Chunk<32, 16, 8>
	{
	private:
		int _value = 0;

	public:
		explicit TestChunk(int value = 0) :
			_value(value)
		{
		}

		void setValue(int value)
		{
			if (value != _value)
			{
				_value = value;
				notifyChunkEdition();
			}
		}

		[[nodiscard]] int value() const noexcept
		{
			return _value;
		}
	};

	class MarkerComponent : public spk::Component3D
	{
	public:
		MarkerComponent() :
			Component3D("Marker")
		{
		}
	};
}

TEST(ChunkCollectionTest, OwnsChunkByValueAndHeadlessEntitiesWithFilters)
{
	spk::ChunkCollection<TestChunk> collection;
	const Coordinate coordinate{-2, 3, 4};
	collection.addChunk(coordinate, TestChunk(42));

	ASSERT_EQ(collection.size(), 1u);
	ASSERT_EQ(collection.root().children().size(), 1u);
	ASSERT_TRUE(collection.contains(coordinate));
	EXPECT_EQ(collection.at(coordinate).value(), 42);

	auto &[key, entry] = *collection.begin();
	EXPECT_EQ(key, coordinate);
	TestChunk *chunk = nullptr;
	EXPECT_TRUE(collection.tryGet(coordinate, chunk));
	EXPECT_EQ(&entry.chunk(), chunk);
	EXPECT_EQ(entry.entity().parent(), &collection.root());
	EXPECT_EQ(entry.entity().transform().position(), (spk::Vector3{-64.0f, 48.0f, 32.0f}));

	const auto *filter = entry.entity().getComponent<spk::ChunkFilter<TestChunk>>();
	ASSERT_NE(filter, nullptr);
	const TestChunk *filteredChunk = nullptr;
	EXPECT_TRUE(filter->tryGet(filteredChunk));
	EXPECT_EQ(filteredChunk, &entry.chunk());
	EXPECT_EQ(&filter->chunk(), &entry.chunk());
	EXPECT_EQ(filter->owner(), &entry.entity());

	const auto &immutable = std::as_const(collection);
	EXPECT_EQ(immutable.at(coordinate).value(), 42);
	EXPECT_EQ(immutable[coordinate].value(), 42);
	const TestChunk *constChunk = nullptr;
	EXPECT_TRUE(immutable.tryGet(coordinate, constChunk));
	EXPECT_EQ(constChunk, &entry.chunk());
	EXPECT_EQ(immutable.begin()->second.chunk().value(), 42);
}

TEST(ChunkCollectionTest, MissingCoordinateDoesNotCreateAnEntry)
{
	spk::ChunkCollection<TestChunk> collection;
	const Coordinate missing{1, 0, -1};
	EXPECT_FALSE(collection.contains(missing));
	TestChunk *chunk = nullptr;
	EXPECT_FALSE(collection.tryGet(missing, chunk));
	EXPECT_EQ(chunk, nullptr);
	const TestChunk *constChunk = nullptr;
	EXPECT_FALSE(std::as_const(collection).tryGet(missing, constChunk));
	EXPECT_EQ(constChunk, nullptr);
	EXPECT_THROW((void)collection.at(missing), spk::Exception);
	EXPECT_THROW((void)collection[missing], spk::Exception);
	EXPECT_THROW((void)std::as_const(collection).at(missing), spk::Exception);
	EXPECT_THROW((void)std::as_const(collection)[missing], spk::Exception);
	EXPECT_TRUE(collection.empty());
}

TEST(ChunkCollectionTest, TryGetReturnsStoredAddressAndClearsOutputOnMiss)
{
	spk::ChunkCollection<TestChunk> collection;
	const Coordinate present{2, 1, -3};
	const Coordinate missing{3, 1, -3};
	collection.addChunk(present, TestChunk(5));

	TestChunk *chunk = nullptr;
	EXPECT_TRUE(collection.tryGet(present, chunk));
	ASSERT_NE(chunk, nullptr);
	EXPECT_EQ(chunk->value(), 5);
	EXPECT_EQ(chunk, &collection.at(present));

	EXPECT_FALSE(collection.tryGet(missing, chunk));
	EXPECT_EQ(chunk, nullptr);

	const TestChunk *constChunk = nullptr;
	const auto &immutable = std::as_const(collection);
	EXPECT_TRUE(immutable.tryGet(present, constChunk));
	ASSERT_NE(constChunk, nullptr);
	EXPECT_EQ(constChunk, &immutable.at(present));
	EXPECT_FALSE(immutable.tryGet(missing, constChunk));
	EXPECT_EQ(constChunk, nullptr);
}

TEST(ChunkCollectionTest, MapIterationIsLexicographicAndNonOwning)
{
	spk::ChunkCollection<TestChunk> collection;
	collection.addChunk({1, 0, 0}, TestChunk(3));
	collection.addChunk({-1, 0, 0}, TestChunk(1));
	collection.addChunk({0, 0, 0}, TestChunk(2));
	std::vector<int> values;
	for (const auto &[coordinate, entry] : collection)
	{
		(void)coordinate;
		values.push_back(entry.chunk().value());
	}
	EXPECT_EQ(values, (std::vector<int>{1, 2, 3}));
	EXPECT_EQ(collection.rbegin()->second.chunk().value(), 3);
	EXPECT_EQ(std::as_const(collection).rbegin()->second.chunk().value(), 3);
}

TEST(ChunkCollectionTest, PublishesAdditionReplacementAndSuccessfulEditsImmediately)
{
	spk::ChunkCollection<TestChunk> collection;
	const Coordinate coordinate{1, 2, 3};
	std::vector<Coordinate> events;
	auto subscription = collection.subscribeToChunkEdition([&](const Coordinate &edited) {
		events.push_back(edited);
		EXPECT_TRUE(collection.contains(edited));
	});

	collection.addChunk(coordinate, TestChunk(1));
	collection.at(coordinate).setValue(2);
	collection.at(coordinate).setValue(2);
	collection.addChunk(coordinate, TestChunk(3));
	collection.at(coordinate).setValue(4);

	EXPECT_EQ(events, (std::vector<Coordinate>{coordinate, coordinate, coordinate, coordinate}));
	EXPECT_EQ(collection.at(coordinate).value(), 4);
	EXPECT_TRUE(subscription.isValid());
}

TEST(ChunkCollectionTest, ReplacementPreservesChunkEntityComponentsAndSubscriptions)
{
	spk::ChunkCollection<TestChunk> collection;
	const Coordinate coordinate{4, -1, 2};
	collection.addChunk(coordinate, TestChunk(1));
	auto &entry = collection.begin()->second;
	TestChunk *chunk = &entry.chunk();
	spk::Entity3D *entity = &entry.entity();
	auto *filter = entity->getComponent<spk::ChunkFilter<TestChunk>>();
	MarkerComponent *marker = &entity->addComponent<MarkerComponent>();
	int destructionCalls = 0;
	int directEdits = 0;
	auto destruction = entity->subscribeToDestruction([&]() {
		++destructionCalls;
	});
	auto direct = chunk->subscribeToChunkEdition([&]() {
		++directEdits;
	});

	collection.addChunk(coordinate, TestChunk(99));

	TestChunk *located = nullptr;
	EXPECT_TRUE(collection.tryGet(coordinate, located));
	EXPECT_EQ(located, chunk);
	EXPECT_EQ(&collection.begin()->second.entity(), entity);
	EXPECT_EQ(entity->getComponent<spk::ChunkFilter<TestChunk>>(), filter);
	EXPECT_EQ(entity->getComponent<MarkerComponent>(), marker);
	TestChunk *filteredChunk = nullptr;
	EXPECT_TRUE(filter->tryGet(filteredChunk));
	EXPECT_EQ(filteredChunk, chunk);
	EXPECT_EQ(chunk->value(), 99);
	EXPECT_EQ(destructionCalls, 0);
	EXPECT_TRUE(destruction.isValid());
	EXPECT_TRUE(direct.isValid());
	chunk->setValue(100);
	EXPECT_EQ(directEdits, 1);
}

TEST(ChunkCollectionTest, RemovingUnbindsFilterBeforeEntityDestruction)
{
	spk::ChunkCollection<TestChunk> collection;
	const Coordinate coordinate{1, 1, 1};
	collection.addChunk(coordinate, TestChunk(8));
	spk::Entity3D &entity = collection.begin()->second.entity();
	auto *filter = entity.getComponent<spk::ChunkFilter<TestChunk>>();
	ASSERT_NE(filter, nullptr);
	TestChunk *originalChunk = &collection.at(coordinate);
	int calls = 0;
	auto destruction = entity.subscribeToDestruction([&]() {
		++calls;
		EXPECT_FALSE(collection.contains(coordinate));
		TestChunk *located = nullptr;
		EXPECT_FALSE(collection.tryGet(coordinate, located));
		EXPECT_EQ(located, nullptr);
		TestChunk *filteredChunk = originalChunk;
		EXPECT_FALSE(filter->tryGet(filteredChunk));
		EXPECT_EQ(filteredChunk, nullptr);
		EXPECT_THROW((void)filter->chunk(), spk::Exception);
		EXPECT_NE(entity.getComponent<MarkerComponent>(), nullptr);
	});
	entity.addComponent<MarkerComponent>();

	EXPECT_TRUE(collection.removeChunk(coordinate));
	EXPECT_FALSE(collection.removeChunk(coordinate));
	EXPECT_TRUE(collection.empty());
	EXPECT_TRUE(collection.root().children().empty());
	EXPECT_EQ(calls, 1);
	EXPECT_FALSE(destruction.isValid());
}

TEST(ChunkCollectionTest, CanReenterCollectionDuringEditionOrRemoval)
{
	spk::ChunkCollection<TestChunk> collection;
	const Coordinate first{1, 0, 0};
	const Coordinate second{2, 0, 0};
	const Coordinate third{3, 0, 0};
	std::vector<Coordinate> notified;
	auto edition = collection.subscribeToChunkEdition([&](const Coordinate &coordinate) {
		notified.push_back(coordinate);
		if (coordinate == first)
		{
			collection.addChunk(second, TestChunk(2));
		}
	});
	collection.addChunk(first, TestChunk(1));
	EXPECT_EQ(notified, (std::vector<Coordinate>{first, second}));

	spk::Entity3D &entity = collection.begin()->second.entity();
	auto removal = entity.subscribeToDestruction([&]() {
		collection.addChunk(third, TestChunk(3));
	});
	EXPECT_TRUE(collection.removeChunk(first));
	EXPECT_FALSE(collection.contains(first));
	EXPECT_TRUE(collection.contains(second));
	EXPECT_TRUE(collection.contains(third));
}

TEST(ChunkCollectionTest, CopiesOfChunksDoNotShareCollectionSubscriptions)
{
	spk::ChunkCollection<TestChunk> collection;
	const Coordinate coordinate{0, 0, 0};
	collection.addChunk(coordinate, TestChunk(3));
	int calls = 0;
	auto edition = collection.subscribeToChunkEdition([&](const Coordinate &) {
		++calls;
	});

	TestChunk independent(collection.at(coordinate));
	independent.setValue(4);
	EXPECT_EQ(calls, 0);
	collection.at(coordinate).setValue(5);
	EXPECT_EQ(calls, 1);

	collection.addChunk(coordinate, independent);
	EXPECT_EQ(calls, 2);
	independent.setValue(6);
	EXPECT_EQ(calls, 2);
	collection.at(coordinate).setValue(7);
	EXPECT_EQ(calls, 3);
}

TEST(ChunkCollectionTest, ExplicitAttachDetachAndRebindPreserveEntries)
{
	spk::EngineHeadless firstEngine;
	spk::EngineHeadless secondEngine;
	spk::ChunkCollection<TestChunk> collection;
	const Coordinate coordinate{1, -2, 3};
	collection.addChunk(coordinate, TestChunk(17));
	spk::Entity3D *entity = &collection.begin()->second.entity();
	auto *filter = entity->getComponent<spk::ChunkFilter<TestChunk>>();
	EXPECT_EQ(collection.engine(), nullptr);

	collection.attach(firstEngine);
	EXPECT_EQ(collection.engine(), &firstEngine);
	EXPECT_EQ(collection.root().parent(), &firstEngine.root());
	EXPECT_EQ(entity->context(), &firstEngine);
	EXPECT_EQ(filter->context(), &firstEngine);
	collection.attach(firstEngine);

	collection.attach(secondEngine);
	EXPECT_EQ(collection.engine(), &secondEngine);
	EXPECT_EQ(entity, &collection.begin()->second.entity());
	EXPECT_EQ(filter->context(), &secondEngine);
	EXPECT_FALSE((spk::Registry<spk::Entity3D, spk::EngineHeadless *>::instance().elements(&firstEngine).contains(entity)));
	EXPECT_TRUE((spk::Registry<spk::Entity3D, spk::EngineHeadless *>::instance().elements(&secondEngine).contains(entity)));

	collection.detach();
	EXPECT_EQ(collection.engine(), nullptr);
	EXPECT_EQ(entity->context(), nullptr);
	EXPECT_EQ(filter->context(), nullptr);
	EXPECT_TRUE(collection.contains(coordinate));
	EXPECT_EQ(collection.at(coordinate).value(), 17);
}

TEST(ChunkCollectionTest, OutlivingAnEngineDetachesWithoutDestroyingChunks)
{
	spk::ChunkCollection<TestChunk> collection;
	const Coordinate coordinate{0, 0, 0};
	collection.addChunk(coordinate, TestChunk(5));
	{
		spk::EngineHeadless engine;
		collection.attach(engine);
		EXPECT_EQ(collection.engine(), &engine);
	}
	EXPECT_EQ(collection.engine(), nullptr);
	EXPECT_EQ(collection.at(coordinate).value(), 5);
	EXPECT_TRUE(collection.removeChunk(coordinate));
}
