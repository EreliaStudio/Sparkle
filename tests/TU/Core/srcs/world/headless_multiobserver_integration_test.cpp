#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <exception>
#include <future>
#include <map>
#include <mutex>
#include <memory>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include "core/context/update_context.hpp"
#include "engine/component3d.hpp"
#include "engine/engine_headless.hpp"
#include "engine/entity3d.hpp"
#include "engine/transform3d.hpp"
#include "threading/task.hpp"
#include "threading/worker_pool.hpp"
#include "world/chunk.hpp"
#include "world/chunk_collection.hpp"
#include "world/chunk_filter.hpp"
#include "world/chunk_interest.hpp"
#include "world/chunk_requester.hpp"
#include "world/generator.hpp"
#include "world/local_chunk_requester.hpp"
#include "world/world_observer.hpp"
#include "world/world_streaming_system.hpp"

namespace
{
	using namespace std::chrono_literals;
	using Coordinate = spk::World::ChunkCoordinate;

	class IntegrationChunk : public spk::World::Chunk<16, 8, 32>
	{
	public:
	private:
		int _value = 0;

	public:
		explicit IntegrationChunk(int value = 0) :
			_value(value)
		{
		}

		[[nodiscard]] int value() const noexcept
		{
			return _value;
		}

		void setValue(int value)
		{
			if (_value != value)
			{
				_value = value;
				notifyChunkEdition();
			}
		}
	};

	struct GenerationData
	{
		void flush()
		{
		}

		std::mutex mutex;
		std::map<Coordinate, int> values;

		int &value(const Coordinate &coordinate)
		{
			const std::scoped_lock lock(mutex);
			return values.at(coordinate);
		}
	};

	using Generator = spk::World::Generator<IntegrationChunk, GenerationData>;
	using Result = std::optional<IntegrationChunk>;
	using Task = spk::Task<Result>;
	using Answer = Task::Answer;
	using Status = Task::Status;
	using Streamer = spk::World::WorldStreamingSystem<IntegrationChunk>;
	using Collection = spk::World::ChunkCollection<IntegrationChunk>;

	class IntegrationGenerator : public Generator
	{
	private:
		[[nodiscard]] bool _prepareGenerationData(const Coordinate &coordinate, GenerationData &data) override
		{
			if (coordinate == Coordinate{9, 0, 0})
			{
				return false;
			}
			const std::scoped_lock lock(data.mutex);
			if (!data.values.try_emplace(coordinate, 0).second)
			{
				throw spk::Exception("Chunk generation is already active");
			}
			return true;
		}

		[[nodiscard]] IntegrationChunk _instantiateChunk(const Coordinate &coordinate, GenerationData &data) override
		{
			return IntegrationChunk(data.value(coordinate));
		}
		void _cleanGenerationData(const Coordinate &coordinate, GenerationData &data) noexcept override
		{
			const std::scoped_lock lock(data.mutex);
			data.values.erase(coordinate);
		}
	};

	class FirstPass : public Generator::Pass
	{
	public:
		void apply(const Coordinate &coordinate, GenerationData &data) override
		{
			data.value(coordinate) = 200 + coordinate.x * 11 + coordinate.y * 13 + coordinate.z * 17;
		}
	};

	class SecondPass : public Generator::Pass
	{
	public:
		void apply(const Coordinate &coordinate, GenerationData &data) override
		{
			data.value(coordinate) = data.value(coordinate) * 3 + coordinate.x - coordinate.y * 2 + coordinate.z;
		}
	};

	[[nodiscard]] int expectedValue(const Coordinate &coordinate)
	{
		return (200 + coordinate.x * 11 + coordinate.y * 13 + coordinate.z * 17) * 3 +
			   coordinate.x - coordinate.y * 2 + coordinate.z;
	}

	class MarkerComponent : public spk::Component3D
	{
	public:
		MarkerComponent() :
			spk::Component3D("Headless marker")
		{
		}
	};

	class ControlledRequester : public spk::World::ChunkRequester<IntegrationChunk>
	{
	public:
		struct Request
		{
			Coordinate coordinate;
			std::shared_ptr<Task> task;
		};

		std::vector<Request> requests;

		[[nodiscard]] Answer request(const Coordinate &coordinate) override
		{
			auto task = std::make_shared<Task>();
			Answer answer = task->answer();
			requests.push_back({coordinate, std::move(task)});
			return answer;
		}

		void complete(std::size_t index, int value)
		{
			requests.at(index).task->validate(Result(IntegrationChunk(value)));
		}

		void absent(std::size_t index)
		{
			requests.at(index).task->validate(std::nullopt);
		}

		void fail(std::size_t index)
		{
			requests.at(index).task->fail(std::make_exception_ptr(std::runtime_error("External source failed")));
		}
	};

	class ControlledFixture
	{
	public:
		spk::EngineHeadless engine;
		Collection collection;
		ControlledRequester requester;

		ControlledFixture()
		{
			collection.attach(engine);
		}

		~ControlledFixture()
		{
			if (Streamer *streamer = engine.getSystem<Streamer>(); streamer != nullptr)
			{
				engine.removeSystem(*streamer);
			}
			collection.detach();
		}

		Streamer &start(Streamer::Settings settings = {})
		{
			return engine.addSystem<Streamer>(collection, requester, settings);
		}

		void tick(std::chrono::milliseconds time)
		{
			spk::UpdateContext context{.time = time, .deltaTime = 1ms};
			engine.updateState(context);
		}
	};

	spk::World::WorldObserver &addObserver(
		spk::EngineHeadless &engine,
		spk::Entity3D &entity,
		const spk::Vector3 &position,
		const spk::Vector3Int &radius = {0, 0, 0})
	{
		entity.transform().place(position);
		auto &observer = entity.addComponent<spk::World::WorldObserver>(spk::World::ChunkInterest(radius));
		engine.addEntity(&entity);
		return observer;
	}

	[[nodiscard]] Collection::ChunkEntry &entryAt(Collection &collection, const Coordinate &coordinate)
	{
		auto it = std::find_if(collection.begin(), collection.end(), [&coordinate](const auto &entry) {
			return entry.first == coordinate;
		});
		if (it == collection.end())
		{
			throw std::runtime_error("Expected loaded chunk");
		}
		return it->second;
	}

	[[nodiscard]] bool waitForCompletion(const Answer &answer)
	{
		const auto deadline = std::chrono::steady_clock::now() + 5s;
		while (answer.status() == Status::Pending && std::chrono::steady_clock::now() < deadline)
		{
			std::this_thread::yield();
		}
		return answer.status() != Status::Pending;
	}
}

TEST(HeadlessMultiObserverIntegrationTest, OverlapMovementRadiusAndChunkEntityLifetime)
{
	ControlledFixture fixture;
	spk::Entity3D first("First observer");
	spk::Entity3D second("Second observer");
	spk::Entity3D ordinary("Non observer");
	auto &firstObserver = addObserver(fixture.engine, first, {0.0f, 0.0f, 0.0f}, {1, 0, 0});
	addObserver(fixture.engine, second, {16.0f, 0.0f, 0.0f}, {1, 0, 0});
	fixture.engine.addEntity(&ordinary);
	Streamer &streamer = fixture.start({.maxInFlight = 8});

	fixture.tick(0ms);
	ASSERT_EQ(fixture.requester.requests.size(), 4u);
	EXPECT_EQ(fixture.requester.requests[0].coordinate, (Coordinate{0, 0, 0}));
	EXPECT_EQ(fixture.requester.requests[1].coordinate, (Coordinate{1, 0, 0}));
	EXPECT_EQ(fixture.requester.requests[2].coordinate, (Coordinate{-1, 0, 0}));
	EXPECT_EQ(fixture.requester.requests[3].coordinate, (Coordinate{2, 0, 0}));
	EXPECT_EQ(streamer.planRevision(), 1u);

	const std::thread::id mainThread = std::this_thread::get_id();
	std::vector<Coordinate> editions;
	auto subscription = fixture.collection.subscribeToChunkEdition([&](const Coordinate &coordinate) {
		EXPECT_EQ(std::this_thread::get_id(), mainThread);
		editions.push_back(coordinate);
	});

	fixture.requester.complete(3, 22);
	fixture.requester.complete(0, 10);
	fixture.requester.absent(1);
	fixture.requester.complete(2, -10);
	EXPECT_TRUE(fixture.collection.empty());
	fixture.tick(1ms);
	ASSERT_EQ(fixture.collection.size(), 3u);
	EXPECT_FALSE(fixture.collection.contains({1, 0, 0}));
	EXPECT_EQ(editions.size(), 3u);

	auto &entry = entryAt(fixture.collection, {-1, 0, 0});
	EXPECT_EQ(entry.chunk().value(), -10);
	EXPECT_EQ(entry.entity().context(), &fixture.engine);
	auto *filter = entry.entity().getComponent<spk::World::ChunkFilter<IntegrationChunk>>();
	ASSERT_NE(filter, nullptr);
	EXPECT_EQ(&filter->chunk(), &entry.chunk());

	auto &centerEntry = entryAt(fixture.collection, {0, 0, 0});
	spk::Entity3D *centerEntity = &centerEntry.entity();
	IntegrationChunk *centerChunk = &centerEntry.chunk();
	auto *centerFilter = centerEntity->getComponent<spk::World::ChunkFilter<IntegrationChunk>>();
	ASSERT_NE(centerFilter, nullptr);
	auto *marker = &centerEntity->addComponent<MarkerComponent>();
	int directEdits = 0;
	auto direct = centerChunk->subscribeToChunkEdition([&]() {
		++directEdits;
	});
	fixture.collection.addChunk({0, 0, 0}, IntegrationChunk(99));
	EXPECT_EQ(&fixture.collection.at({0, 0, 0}), centerChunk);
	EXPECT_EQ(&centerEntry.entity(), centerEntity);
	EXPECT_EQ(centerEntity->getComponent<MarkerComponent>(), marker);
	EXPECT_EQ(centerEntity->getComponent<spk::World::ChunkFilter<IntegrationChunk>>(), centerFilter);
	EXPECT_EQ(&centerFilter->chunk(), centerChunk);
	centerChunk->setValue(100);
	EXPECT_EQ(directEdits, 1);
	EXPECT_EQ(editions.size(), 5u);

	ordinary.transform().place({2048.0f, -128.0f, 4096.0f});
	first.transform().place({15.5f, 7.9f, 31.9f});
	fixture.tick(2ms);
	EXPECT_EQ(streamer.planRevision(), 1u);
	EXPECT_EQ(fixture.requester.requests.size(), 4u);

	auto &distantEntry = entryAt(fixture.collection, {2, 0, 0});
	int destructions = 0;
	auto destroyed = distantEntry.entity().subscribeToDestruction([&]() {
		++destructions;
		EXPECT_FALSE(fixture.collection.contains({2, 0, 0}));
	});
	fixture.engine.removeEntity(&second);
	fixture.tick(3ms);
	EXPECT_EQ(streamer.planRevision(), 2u);
	EXPECT_FALSE(fixture.collection.contains({2, 0, 0}));
	EXPECT_EQ(destructions, 1);
	EXPECT_FALSE(destroyed.isValid());
	EXPECT_TRUE(fixture.collection.contains({-1, 0, 0}));
	EXPECT_TRUE(fixture.collection.contains({0, 0, 0}));

	firstObserver.setInterest(spk::World::ChunkInterest({2, 0, 0}));
	fixture.tick(4ms);
	EXPECT_EQ(streamer.planRevision(), 3u);
	ASSERT_EQ(fixture.requester.requests.size(), 6u);
	EXPECT_EQ(fixture.requester.requests[4].coordinate, (Coordinate{-2, 0, 0}));
	EXPECT_EQ(fixture.requester.requests[5].coordinate, (Coordinate{2, 0, 0}));
	EXPECT_TRUE(subscription.isValid());
	EXPECT_TRUE(direct.isValid());
}

TEST(HeadlessMultiObserverIntegrationTest, SeparatingObserversExpandsInterestAndForgetsOnlyExitedAbsences)
{
	ControlledFixture fixture;
	spk::Entity3D first("First");
	spk::Entity3D second("Second");
	addObserver(fixture.engine, first, {0.0f, 0.0f, 0.0f}, {1, 0, 0});
	addObserver(fixture.engine, second, {0.0f, 0.0f, 0.0f}, {1, 0, 0});
	Streamer &streamer = fixture.start({.maxInFlight = 8});

	fixture.tick(0ms);
	ASSERT_EQ(fixture.requester.requests.size(), 3u);
	EXPECT_EQ(fixture.requester.requests[0].coordinate, (Coordinate{0, 0, 0}));
	EXPECT_EQ(fixture.requester.requests[1].coordinate, (Coordinate{-1, 0, 0}));
	EXPECT_EQ(fixture.requester.requests[2].coordinate, (Coordinate{1, 0, 0}));
	for (std::size_t index = 0; index < 3; ++index)
	{
		fixture.requester.absent(index);
	}
	fixture.tick(1ms);

	second.transform().place({48.0f, 0.0f, 0.0f});
	fixture.tick(2ms);
	ASSERT_EQ(fixture.requester.requests.size(), 6u);
	EXPECT_EQ(fixture.requester.requests[3].coordinate, (Coordinate{3, 0, 0}));
	EXPECT_EQ(fixture.requester.requests[4].coordinate, (Coordinate{2, 0, 0}));
	EXPECT_EQ(fixture.requester.requests[5].coordinate, (Coordinate{4, 0, 0}));
	for (std::size_t index = 3; index < 6; ++index)
	{
		fixture.requester.absent(index);
	}
	fixture.tick(3ms);

	second.transform().place({0.0f, 0.0f, 0.0f});
	fixture.tick(4ms);
	EXPECT_EQ(fixture.requester.requests.size(), 6u);
	second.transform().place({48.0f, 0.0f, 0.0f});
	fixture.tick(5ms);
	ASSERT_EQ(fixture.requester.requests.size(), 9u);
	EXPECT_EQ(fixture.requester.requests[6].coordinate, (Coordinate{3, 0, 0}));
	EXPECT_EQ(fixture.requester.requests[7].coordinate, (Coordinate{2, 0, 0}));
	EXPECT_EQ(fixture.requester.requests[8].coordinate, (Coordinate{4, 0, 0}));
	EXPECT_EQ(streamer.planRevision(), 4u);
}

TEST(HeadlessMultiObserverIntegrationTest, ReprioritizesOnlyUnsubmittedChunksByNearestInterestedObserver)
{
	ControlledFixture fixture;
	spk::Entity3D first("First");
	spk::Entity3D second("Second");
	addObserver(fixture.engine, first, {0.0f, 0.0f, 0.0f}, {2, 0, 0});
	addObserver(fixture.engine, second, {48.0f, 0.0f, 0.0f}, {1, 0, 0});
	Streamer &streamer = fixture.start({.maxInFlight = 1});

	fixture.tick(0ms);
	ASSERT_EQ(fixture.requester.requests.size(), 1u);
	EXPECT_EQ(fixture.requester.requests[0].coordinate, (Coordinate{0, 0, 0}));

	first.transform().place({16.0f, 0.0f, 0.0f});
	second.transform().place({64.0f, 0.0f, 0.0f});
	fixture.tick(1ms);
	EXPECT_EQ(streamer.planRevision(), 2u);
	EXPECT_EQ(streamer.inFlightCount(), 1u);
	EXPECT_EQ(fixture.requester.requests.size(), 1u);

	const std::vector<Coordinate> expected{
		{0, 0, 0}, {1, 0, 0}, {4, 0, 0}, {2, 0, 0}, {3, 0, 0}, {5, 0, 0}, {-1, 0, 0}};
	for (std::size_t index = 0; index + 1 < expected.size(); ++index)
	{
		fixture.requester.absent(index);
		fixture.tick(std::chrono::milliseconds(static_cast<int>(index + 2)));
		ASSERT_EQ(fixture.requester.requests.size(), index + 2);
		EXPECT_EQ(fixture.requester.requests.back().coordinate, expected[index + 1]);
	}
	EXPECT_EQ(streamer.planRevision(), 2u);
	EXPECT_TRUE(fixture.collection.empty());
}

TEST(HeadlessMultiObserverIntegrationTest, RemoteFailureTimeoutAbsenceAndInterestReentry)
{
	ControlledFixture fixture;
	spk::Entity3D player("Player");
	addObserver(fixture.engine, player, {0.0f, 0.0f, 0.0f});
	Streamer &streamer = fixture.start({.maxInFlight = 1, .timeout = 10ms, .retryDelay = 5ms});

	fixture.tick(0ms);
	ASSERT_EQ(fixture.requester.requests.size(), 1u);
	fixture.tick(10ms);
	EXPECT_EQ(streamer.inFlightCount(), 1u);
	fixture.requester.complete(0, 999);
	fixture.tick(11ms);
	EXPECT_TRUE(fixture.collection.empty());
	EXPECT_EQ(streamer.inFlightCount(), 0u);
	fixture.tick(14ms);
	EXPECT_EQ(fixture.requester.requests.size(), 1u);

	fixture.tick(15ms);
	ASSERT_EQ(fixture.requester.requests.size(), 2u);
	fixture.requester.fail(1);
	fixture.tick(16ms);
	EXPECT_TRUE(fixture.collection.empty());
	fixture.tick(20ms);
	EXPECT_EQ(fixture.requester.requests.size(), 2u);

	fixture.tick(21ms);
	ASSERT_EQ(fixture.requester.requests.size(), 3u);
	fixture.requester.absent(2);
	fixture.tick(22ms);
	fixture.tick(40ms);
	EXPECT_EQ(fixture.requester.requests.size(), 3u);
	EXPECT_TRUE(fixture.collection.empty());

	player.transform().place({16.0f, 0.0f, 0.0f});
	fixture.tick(41ms);
	ASSERT_EQ(fixture.requester.requests.size(), 4u);
	fixture.requester.absent(3);
	fixture.tick(42ms);
	player.transform().place({0.0f, 0.0f, 0.0f});
	fixture.tick(43ms);
	ASSERT_EQ(fixture.requester.requests.size(), 5u);
	EXPECT_EQ(fixture.requester.requests.back().coordinate, (Coordinate{0, 0, 0}));
	fixture.requester.complete(4, 123);
	fixture.tick(44ms);
	EXPECT_EQ(fixture.collection.at({0, 0, 0}).value(), 123);
}

TEST(HeadlessMultiObserverIntegrationTest, PhysicalBudgetRejectsStaleCompletionAfterLeavingAndReentering)
{
	ControlledFixture fixture;
	spk::Entity3D player("Player");
	addObserver(fixture.engine, player, {-0.1f, -8.1f, -32.1f});
	Streamer &streamer = fixture.start({.maxInFlight = 1, .timeout = 10ms, .retryDelay = 5ms});

	fixture.tick(0ms);
	ASSERT_EQ(fixture.requester.requests.size(), 1u);
	EXPECT_EQ(fixture.requester.requests[0].coordinate, (Coordinate{-1, -2, -2}));
	fixture.tick(10ms);
	player.transform().place({0.0f, 0.0f, 0.0f});
	fixture.tick(11ms);
	player.transform().place({-0.1f, -8.1f, -32.1f});
	fixture.tick(12ms);
	fixture.tick(100ms);
	EXPECT_EQ(streamer.inFlightCount(), 1u);
	EXPECT_EQ(fixture.requester.requests.size(), 1u);
	EXPECT_TRUE(fixture.collection.empty());

	fixture.requester.complete(0, 999);
	fixture.tick(101ms);
	ASSERT_EQ(fixture.requester.requests.size(), 2u);
	EXPECT_EQ(fixture.requester.requests[1].coordinate, (Coordinate{-1, -2, -2}));
	EXPECT_FALSE(fixture.collection.contains({-1, -2, -2}));
	fixture.requester.complete(1, 81);
	fixture.tick(102ms);
	EXPECT_EQ(fixture.collection.at({-1, -2, -2}).value(), 81);
	EXPECT_EQ(streamer.inFlightCount(), 0u);
}

TEST(HeadlessMultiObserverIntegrationTest, LocalWorkersGenerateDeterministicChunksWithoutMutatingHeadlessEntities)
{
	spk::EngineHeadless engine;
	Collection collection;
	IntegrationGenerator generator;
	generator.addPass<FirstPass>();
	generator.addPass<SecondPass>();
	spk::WorkerPool workers(2u);
	spk::World::LocalChunkRequester<IntegrationChunk, GenerationData> requester(workers, generator);
	collection.attach(engine);
	spk::Entity3D first("First");
	spk::Entity3D second("Second");
	spk::Entity3D ordinary("Ordinary");
	addObserver(engine, first, {0.0f, 0.0f, 0.0f}, {1, 0, 0});
	addObserver(engine, second, {48.0f, 0.0f, 0.0f});
	engine.addEntity(&ordinary);

	const std::thread::id mainThread = std::this_thread::get_id();
	std::vector<Coordinate> editions;
	auto contract = collection.subscribeToChunkEdition([&](const Coordinate &coordinate) {
		EXPECT_EQ(std::this_thread::get_id(), mainThread);
		editions.push_back(coordinate);
	});
	Streamer &streamer = engine.addSystem<Streamer>(collection, requester, Streamer::Settings{.maxInFlight = 4});
	const std::vector<Coordinate> coordinates{{-1, 0, 0}, {0, 0, 0}, {1, 0, 0}, {3, 0, 0}};
	const auto deadline = std::chrono::steady_clock::now() + 5s;
	while (collection.size() != coordinates.size() && std::chrono::steady_clock::now() < deadline)
	{
		spk::UpdateContext context{.time = 1ms, .deltaTime = 1ms};
		engine.updateState(context);
		std::this_thread::yield();
	}
	ASSERT_EQ(collection.size(), coordinates.size());
	EXPECT_EQ(streamer.inFlightCount(), 0u);
	EXPECT_EQ(editions.size(), coordinates.size());

	std::map<Coordinate, int> values;
	for (const Coordinate &coordinate : coordinates)
	{
		auto &entry = entryAt(collection, coordinate);
		EXPECT_EQ(entry.entity().context(), &engine);
		auto *filter = entry.entity().getComponent<spk::World::ChunkFilter<IntegrationChunk>>();
		ASSERT_NE(filter, nullptr);
		EXPECT_EQ(&filter->chunk(), &entry.chunk());
		EXPECT_EQ(entry.chunk().value(), expectedValue(coordinate));
		values.emplace(coordinate, entry.chunk().value());
	}

	std::vector<Coordinate> reversed = coordinates;
	std::reverse(reversed.begin(), reversed.end());
	for (const Coordinate &coordinate : reversed)
	{
		const auto result = generator.generate(coordinate);
		ASSERT_TRUE(result.has_value());
		EXPECT_EQ(result->value(), values.at(coordinate));
	}
	engine.removeSystem(streamer);
	collection.detach();
	EXPECT_TRUE(contract.isValid());
}

TEST(HeadlessMultiObserverIntegrationTest, LocalWorkerShutdownDoesNotWaitForBlockedGeneration)
{
	class BlockingPass : public Generator::Pass
	{
	private:
		std::promise<void> &_entered;
		std::shared_future<void> _release;

	public:
		BlockingPass(std::promise<void> &entered, std::shared_future<void> release) :
			_entered(entered),
			_release(std::move(release))
		{
		}

		void apply(const Coordinate &coordinate, GenerationData &data) override
		{
			_entered.set_value();
			_release.wait();
			data.value(coordinate) = 77;
		}
	};

	spk::EngineHeadless engine;
	Collection collection;
	IntegrationGenerator generator;
	std::promise<void> entered;
	auto enteredFuture = entered.get_future();
	std::promise<void> release;
	generator.addPass<BlockingPass>(entered, release.get_future().share());
	spk::WorkerPool workers(1u);
	spk::World::LocalChunkRequester<IntegrationChunk, GenerationData> requester(workers, generator);
	collection.attach(engine);
	spk::Entity3D player("Player");
	addObserver(engine, player, {0.0f, 0.0f, 0.0f});
	Streamer &streamer = engine.addSystem<Streamer>(collection, requester, Streamer::Settings{.maxInFlight = 1, .timeout = 10ms, .retryDelay = 5ms});

	spk::UpdateContext context{.time = 0ms, .deltaTime = 1ms};
	engine.updateState(context);
	EXPECT_EQ(enteredFuture.wait_for(5s), std::future_status::ready);
	EXPECT_EQ(streamer.inFlightCount(), 1u);
	context.time = 10ms;
	engine.updateState(context);
	EXPECT_EQ(streamer.inFlightCount(), 1u);
	EXPECT_TRUE(collection.empty());

	engine.removeSystem(streamer);
	EXPECT_TRUE(collection.empty());
	collection.detach();
	release.set_value();
	EXPECT_TRUE(collection.empty());
}
