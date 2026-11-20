#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <future>
#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include "core/context/update_context.hpp"
#include "engine/engine_headless.hpp"
#include "engine/entity3d.hpp"
#include "engine/transform3d.hpp"
#include "exception.hpp"
#include "world/chunk.hpp"
#include "world/chunk_collection.hpp"
#include "world/chunk_interest.hpp"
#include "world/chunk_requester.hpp"
#include "world/world_observer.hpp"
#include "world/world_streaming_system.hpp"

namespace
{
	using namespace std::chrono_literals;
	using Coordinate = spk::ChunkCoordinate;

	class TestChunk : public spk::Chunk<16, 8, 32>
	{
	private:
		int _value;

	public:
		explicit TestChunk(int value) :
			_value(value)
		{
		}

		[[nodiscard]] int value() const noexcept
		{
			return _value;
		}
	};

	using Result = std::optional<TestChunk>;
	using Task = spk::Task<Result>;
	using Answer = Task::Answer;
	using Streamer = spk::WorldStreamingSystem<TestChunk>;

	class ControlledRequester : public spk::ChunkRequester<TestChunk>
	{
	public:
		struct Request
		{
			Coordinate coordinate;
			std::shared_ptr<Task> task;
		};

		std::vector<Request> requests;
		bool failSynchronously = false;

		[[nodiscard]] Answer request(const Coordinate &coordinate) override
		{
			if (failSynchronously)
			{
				throw spk::Exception("Remote request submission failed");
			}

			auto task = std::make_shared<Task>();
			Answer answer = task->answer();
			requests.push_back(Request{coordinate, std::move(task)});
			return answer;
		}

		void complete(std::size_t index, int value)
		{
			requests.at(index).task->validate(Result(TestChunk(value)));
		}

		void absent(std::size_t index)
		{
			requests.at(index).task->validate(std::nullopt);
		}

		void fail(std::size_t index)
		{
			requests.at(index).task->fail(std::make_exception_ptr(std::runtime_error("request failed")));
		}
	};

	class ImmediateRequester : public spk::ChunkRequester<TestChunk>
	{
	public:
		std::vector<Coordinate> requested;

		[[nodiscard]] Answer request(const Coordinate &coordinate) override
		{
			Task task;
			requested.push_back(coordinate);
			task.validate(Result(TestChunk(coordinate.x + coordinate.y + coordinate.z)));
			return task.answer();
		}
	};

	class Fixture
	{
	public:
		spk::EngineHeadless engine;
		spk::ChunkCollection<TestChunk> collection;
		ControlledRequester requester;

		Fixture()
		{
			collection.attach(engine);
		}

		~Fixture()
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

	spk::WorldObserver &addObserver(
		spk::EngineHeadless &engine,
		spk::Entity3D &entity,
		const spk::Vector3 &worldPosition,
		const spk::Vector3Int &radius = {0, 0, 0})
	{
		entity.transform().place(worldPosition);
		auto &observer = entity.addComponent<spk::WorldObserver>(spk::ChunkInterest(radius));
		engine.addEntity(&entity);
		return observer;
	}
}

TEST(WorldStreamingSystemTest, EmptyObserverSetEvictsChunksOnFirstUpdate)
{
	Fixture fixture;
	const Coordinate coordinate{2, 0, 0};
	fixture.collection.addChunk(coordinate, TestChunk(5));
	Streamer &streamer = fixture.start();

	fixture.tick(0ms);

	EXPECT_TRUE(fixture.collection.empty());
	EXPECT_TRUE(fixture.requester.requests.empty());
	EXPECT_EQ(streamer.planRevision(), 1u);
	EXPECT_EQ(streamer.inFlightCount(), 0u);
}

TEST(WorldStreamingSystemTest, RequestsCenterBeforeNeighborsAndUsesStableTieOrder)
{
	Fixture fixture;
	spk::Entity3D player("player");
	addObserver(fixture.engine, player, {0.0f, 0.0f, 0.0f}, {2, 0, 0});
	Streamer &streamer = fixture.start({.maxInFlight = 1});

	fixture.tick(0ms);
	ASSERT_EQ(fixture.requester.requests.size(), 1u);
	EXPECT_EQ(fixture.requester.requests[0].coordinate, (Coordinate{0, 0, 0}));

	fixture.requester.absent(0);
	fixture.tick(1ms);
	ASSERT_EQ(fixture.requester.requests.size(), 2u);
	EXPECT_EQ(fixture.requester.requests[1].coordinate, (Coordinate{-1, 0, 0}));

	fixture.requester.absent(1);
	fixture.tick(2ms);
	ASSERT_EQ(fixture.requester.requests.size(), 3u);
	EXPECT_EQ(fixture.requester.requests[2].coordinate, (Coordinate{1, 0, 0}));

	fixture.requester.absent(2);
	fixture.tick(3ms);
	ASSERT_EQ(fixture.requester.requests.size(), 4u);
	EXPECT_EQ(fixture.requester.requests[3].coordinate, (Coordinate{-2, 0, 0}));

	fixture.requester.absent(3);
	fixture.tick(4ms);
	ASSERT_EQ(fixture.requester.requests.size(), 5u);
	EXPECT_EQ(fixture.requester.requests[4].coordinate, (Coordinate{2, 0, 0}));
	EXPECT_EQ(streamer.planRevision(), 1u);
}

TEST(WorldStreamingSystemTest, ManhattanDistanceUsesWorldUnitDimensions)
{
	Fixture fixture;
	spk::Entity3D player("player");
	addObserver(fixture.engine, player, {0.0f, 0.0f, 0.0f}, {1, 1, 1});
	fixture.start({.maxInFlight = 1});

	fixture.tick(0ms);
	EXPECT_EQ(fixture.requester.requests.at(0).coordinate, (Coordinate{0, 0, 0}));
	fixture.requester.absent(0);
	fixture.tick(1ms);
	EXPECT_EQ(fixture.requester.requests.at(1).coordinate, (Coordinate{0, -1, 0}));
	fixture.requester.absent(1);
	fixture.tick(2ms);
	EXPECT_EQ(fixture.requester.requests.at(2).coordinate, (Coordinate{0, 1, 0}));
	fixture.requester.absent(2);
	fixture.tick(3ms);
	EXPECT_EQ(fixture.requester.requests.at(3).coordinate, (Coordinate{-1, 0, 0}));
	fixture.requester.absent(3);
	fixture.tick(4ms);
	EXPECT_EQ(fixture.requester.requests.at(4).coordinate, (Coordinate{1, 0, 0}));
	fixture.requester.absent(4);
	fixture.tick(5ms);
	EXPECT_EQ(fixture.requester.requests.at(5).coordinate, (Coordinate{-1, -1, 0}));
}

TEST(WorldStreamingSystemTest, OnlyChunkCenterChangesRebuildPlan)
{
	Fixture fixture;
	spk::Entity3D player("player");
	auto &observer = addObserver(fixture.engine, player, {-0.5f, 0.0f, 0.0f});
	Streamer &streamer = fixture.start({.maxInFlight = 2});

	fixture.tick(0ms);
	EXPECT_EQ(streamer.planRevision(), 1u);
	EXPECT_EQ(fixture.requester.requests.at(0).coordinate, (Coordinate{-1, 0, 0}));

	player.transform().place({-15.5f, 7.9f, 31.9f});
	fixture.tick(1ms);
	EXPECT_EQ(streamer.planRevision(), 1u);
	EXPECT_EQ(fixture.requester.requests.size(), 1u);

	player.transform().place({-16.1f, 7.9f, 31.9f});
	fixture.tick(2ms);
	EXPECT_EQ(streamer.planRevision(), 2u);
	ASSERT_EQ(fixture.requester.requests.size(), 2u);
	EXPECT_EQ(fixture.requester.requests.at(1).coordinate, (Coordinate{-2, 0, 0}));

	observer.setInterest(spk::ChunkInterest({1, 0, 0}));
	fixture.tick(3ms);
	EXPECT_EQ(streamer.planRevision(), 3u);

	observer.setInterest(spk::ChunkInterest({1, 0, 0}));
	fixture.tick(4ms);
	EXPECT_EQ(streamer.planRevision(), 3u);
}

TEST(WorldStreamingSystemTest, ObserverMembershipAndActivityRebuildUnion)
{
	Fixture fixture;
	spk::Entity3D firstPlayer("first");
	spk::Entity3D secondPlayer("second");
	addObserver(fixture.engine, firstPlayer, {0.0f, 0.0f, 0.0f});
	Streamer &streamer = fixture.start({.maxInFlight = 4});

	fixture.tick(0ms);
	EXPECT_EQ(streamer.planRevision(), 1u);
	EXPECT_EQ(fixture.requester.requests.size(), 1u);

	auto &second = addObserver(fixture.engine, secondPlayer, {64.0f, 0.0f, 0.0f});
	fixture.tick(1ms);
	EXPECT_EQ(streamer.planRevision(), 2u);
	EXPECT_EQ(fixture.requester.requests.size(), 2u);
	EXPECT_EQ(fixture.requester.requests[1].coordinate, (Coordinate{4, 0, 0}));

	second.deactivate();
	fixture.tick(2ms);
	EXPECT_EQ(streamer.planRevision(), 3u);
	second.activate();
	fixture.tick(3ms);
	EXPECT_EQ(streamer.planRevision(), 4u);
	fixture.engine.removeEntity(&secondPlayer);
	fixture.tick(4ms);
	EXPECT_EQ(streamer.planRevision(), 5u);
}

TEST(WorldStreamingSystemTest, TwoObserversShareOneChunkRequest)
{
	Fixture fixture;
	spk::Entity3D a("A");
	spk::Entity3D b("B");
	addObserver(fixture.engine, a, {0.0f, 0.0f, 0.0f}, {1, 0, 0});
	addObserver(fixture.engine, b, {16.0f, 0.0f, 0.0f}, {1, 0, 0});
	fixture.start({.maxInFlight = 8});

	fixture.tick(0ms);

	ASSERT_EQ(fixture.requester.requests.size(), 4u);
	EXPECT_EQ(fixture.requester.requests[0].coordinate, (Coordinate{0, 0, 0}));
	EXPECT_EQ(fixture.requester.requests[1].coordinate, (Coordinate{1, 0, 0}));
	EXPECT_EQ(fixture.requester.requests[2].coordinate, (Coordinate{-1, 0, 0}));
	EXPECT_EQ(fixture.requester.requests[3].coordinate, (Coordinate{2, 0, 0}));
}

TEST(WorldStreamingSystemTest, CompletedValueIsPublishedOnlyDuringEngineUpdate)
{
	Fixture fixture;
	spk::Entity3D player("player");
	addObserver(fixture.engine, player, {0.0f, 0.0f, 0.0f});
	Streamer &streamer = fixture.start({.maxInFlight = 1});
	const std::thread::id engineThread = std::this_thread::get_id();
	std::thread::id callbackThread;
	int editions = 0;
	auto subscription = fixture.collection.subscribeToChunkEdition([&](const Coordinate &) {
		callbackThread = std::this_thread::get_id();
		++editions;
	});

	fixture.tick(0ms);
	auto task = fixture.requester.requests.at(0).task;
	std::thread worker([task]() {
		task->validate(Result(TestChunk(52)));
	});
	worker.join();

	EXPECT_FALSE(fixture.collection.contains({0, 0, 0}));
	EXPECT_EQ(editions, 0);
	fixture.tick(1ms);
	EXPECT_TRUE(fixture.collection.contains({0, 0, 0}));
	EXPECT_EQ(fixture.collection.at({0, 0, 0}).value(), 52);
	EXPECT_EQ(editions, 1);
	EXPECT_EQ(callbackThread, engineThread);
	EXPECT_EQ(streamer.inFlightCount(), 0u);
}

TEST(WorldStreamingSystemTest, EmptyAnswerIsCachedUntilInterestIsExited)
{
	Fixture fixture;
	spk::Entity3D player("player");
	addObserver(fixture.engine, player, {0.0f, 0.0f, 0.0f});
	fixture.start({.maxInFlight = 2});

	fixture.tick(0ms);
	fixture.requester.absent(0);
	fixture.tick(1ms);
	fixture.tick(20ms);
	EXPECT_EQ(fixture.requester.requests.size(), 1u);
	EXPECT_FALSE(fixture.collection.contains({0, 0, 0}));

	player.transform().place({16.0f, 0.0f, 0.0f});
	fixture.tick(21ms);
	EXPECT_EQ(fixture.requester.requests.size(), 2u);
	EXPECT_EQ(fixture.requester.requests[1].coordinate, (Coordinate{1, 0, 0}));

	player.transform().place({0.0f, 0.0f, 0.0f});
	fixture.tick(22ms);
	ASSERT_EQ(fixture.requester.requests.size(), 3u);
	EXPECT_EQ(fixture.requester.requests[2].coordinate, (Coordinate{0, 0, 0}));
}

TEST(WorldStreamingSystemTest, FailedTaskRetriesOnlyAfterConfiguredDelay)
{
	Fixture fixture;
	spk::Entity3D player("player");
	addObserver(fixture.engine, player, {0.0f, 0.0f, 0.0f});
	fixture.start({.maxInFlight = 1, .timeout = 100ms, .retryDelay = 10ms});

	fixture.tick(0ms);
	fixture.requester.fail(0);
	fixture.tick(1ms);
	EXPECT_EQ(fixture.requester.requests.size(), 1u);
	fixture.tick(10ms);
	EXPECT_EQ(fixture.requester.requests.size(), 1u);
	fixture.tick(11ms);
	ASSERT_EQ(fixture.requester.requests.size(), 2u);
	fixture.requester.complete(1, 73);
	fixture.tick(12ms);
	EXPECT_EQ(fixture.collection.at({0, 0, 0}).value(), 73);
}

TEST(WorldStreamingSystemTest, SynchronousSubmissionFailureDoesNotBusyRetry)
{
	Fixture fixture;
	spk::Entity3D player("player");
	addObserver(fixture.engine, player, {0.0f, 0.0f, 0.0f});
	fixture.requester.failSynchronously = true;
	Streamer &streamer = fixture.start({.maxInFlight = 1, .timeout = 100ms, .retryDelay = 10ms});

	EXPECT_NO_THROW(fixture.tick(0ms));
	EXPECT_EQ(streamer.inFlightCount(), 0u);
	fixture.tick(9ms);
	EXPECT_TRUE(fixture.requester.requests.empty());

	fixture.requester.failSynchronously = false;
	fixture.tick(10ms);
	ASSERT_EQ(fixture.requester.requests.size(), 1u);
	EXPECT_EQ(fixture.requester.requests[0].coordinate, (Coordinate{0, 0, 0}));
}

TEST(WorldStreamingSystemTest, OrderedPlanDoesNotSubmitDuplicatePendingRequests)
{
	Fixture fixture;
	spk::Entity3D player("player");
	addObserver(fixture.engine, player, {0.0f, 0.0f, 0.0f});
	Streamer &streamer = fixture.start({.maxInFlight = 4});

	fixture.tick(0ms);
	fixture.tick(1ms);
	fixture.tick(2ms);
	ASSERT_EQ(fixture.requester.requests.size(), 1u);
	EXPECT_EQ(streamer.inFlightCount(), 1u);

	fixture.requester.complete(0, 17);
	fixture.tick(3ms);
	fixture.tick(4ms);
	EXPECT_EQ(fixture.requester.requests.size(), 1u);
	EXPECT_EQ(fixture.collection.at({0, 0, 0}).value(), 17);
}

TEST(WorldStreamingSystemTest, TimedOutRequestMayRetryUnderAvailablePhysicalBudget)
{
	Fixture fixture;
	spk::Entity3D player("player");
	addObserver(fixture.engine, player, {0.0f, 0.0f, 0.0f});
	Streamer &streamer = fixture.start({.maxInFlight = 2, .timeout = 10ms, .retryDelay = 5ms});

	fixture.tick(0ms);
	fixture.tick(10ms);
	EXPECT_EQ(streamer.inFlightCount(), 1u);
	fixture.tick(14ms);
	EXPECT_EQ(fixture.requester.requests.size(), 1u);

	fixture.tick(15ms);
	ASSERT_EQ(fixture.requester.requests.size(), 2u);
	EXPECT_EQ(streamer.inFlightCount(), 2u);

	fixture.requester.complete(0, 100);
	fixture.tick(16ms);
	EXPECT_FALSE(fixture.collection.contains({0, 0, 0}));
	EXPECT_EQ(streamer.inFlightCount(), 1u);

	fixture.requester.complete(1, 200);
	fixture.tick(17ms);
	EXPECT_EQ(fixture.collection.at({0, 0, 0}).value(), 200);
	EXPECT_EQ(streamer.inFlightCount(), 0u);
}

TEST(WorldStreamingSystemTest, ExpiredPhysicalTaskOccupiesBudgetUntilSettled)
{
	Fixture fixture;
	spk::Entity3D player("player");
	addObserver(fixture.engine, player, {0.0f, 0.0f, 0.0f}, {1, 0, 0});
	Streamer &streamer = fixture.start({.maxInFlight = 1, .timeout = 20ms, .retryDelay = 10ms});

	fixture.tick(0ms);
	ASSERT_EQ(fixture.requester.requests.size(), 1u);

	fixture.tick(20ms);
	fixture.tick(30ms);
	fixture.tick(100ms);
	EXPECT_EQ(streamer.inFlightCount(), 1u);
	EXPECT_EQ(fixture.requester.requests.size(), 1u);
	EXPECT_TRUE(fixture.collection.empty());

	fixture.requester.complete(0, 999);
	fixture.tick(101ms);
	EXPECT_FALSE(fixture.collection.contains({0, 0, 0}));
	ASSERT_EQ(fixture.requester.requests.size(), 2u);
	EXPECT_EQ(fixture.requester.requests[1].coordinate, (Coordinate{0, 0, 0}));
	EXPECT_EQ(streamer.inFlightCount(), 1u);
}

TEST(WorldStreamingSystemTest, TimedOutLateAnswerCannotOverwriteSuccessfulRetry)
{
	Fixture fixture;
	spk::Entity3D player("player");
	addObserver(fixture.engine, player, {0.0f, 0.0f, 0.0f});
	fixture.start({.maxInFlight = 2, .timeout = 10ms, .retryDelay = 2ms});

	fixture.tick(0ms);
	fixture.tick(10ms);
	fixture.tick(11ms);
	EXPECT_EQ(fixture.requester.requests.size(), 1u);
	fixture.tick(12ms);
	ASSERT_EQ(fixture.requester.requests.size(), 2u);

	fixture.requester.complete(1, 12);
	fixture.tick(13ms);
	EXPECT_EQ(fixture.collection.at({0, 0, 0}).value(), 12);
	fixture.requester.complete(0, 900);
	fixture.tick(14ms);
	EXPECT_EQ(fixture.collection.at({0, 0, 0}).value(), 12);
}

TEST(WorldStreamingSystemTest, LeavingAndReenteringInvalidatesOldRequestGeneration)
{
	Fixture fixture;
	spk::Entity3D player("player");
	addObserver(fixture.engine, player, {0.0f, 0.0f, 0.0f});
	fixture.start({.maxInFlight = 3});

	fixture.tick(0ms);
	player.transform().place({16.0f, 0.0f, 0.0f});
	fixture.tick(1ms);
	player.transform().place({0.0f, 0.0f, 0.0f});
	fixture.tick(2ms);

	ASSERT_EQ(fixture.requester.requests.size(), 3u);
	EXPECT_EQ(fixture.requester.requests[2].coordinate, (Coordinate{0, 0, 0}));
	fixture.requester.complete(0, 100);
	fixture.requester.complete(1, 200);
	fixture.tick(3ms);
	EXPECT_FALSE(fixture.collection.contains({0, 0, 0}));
	EXPECT_FALSE(fixture.collection.contains({1, 0, 0}));

	fixture.requester.complete(2, 300);
	fixture.tick(4ms);
	EXPECT_EQ(fixture.collection.at({0, 0, 0}).value(), 300);
}

TEST(WorldStreamingSystemTest, OutsideChunksAreEvictedImmediatelyWithTheirEntities)
{
	Fixture fixture;
	spk::Entity3D player("player");
	addObserver(fixture.engine, player, {0.0f, 0.0f, 0.0f});
	fixture.start({.maxInFlight = 1});

	fixture.tick(0ms);
	fixture.requester.complete(0, 30);
	fixture.tick(1ms);
	ASSERT_TRUE(fixture.collection.contains({0, 0, 0}));
	spk::Entity3D *original = &fixture.collection.begin()->second.entity();
	int destroys = 0;
	auto token = original->subscribeToDestruction([&]() {
		++destroys;
	});

	player.transform().place({16.0f, 0.0f, 0.0f});
	fixture.tick(2ms);
	EXPECT_FALSE(fixture.collection.contains({0, 0, 0}));
	EXPECT_EQ(destroys, 1);
	EXPECT_FALSE(token.isValid());
	ASSERT_EQ(fixture.requester.requests.size(), 2u);
	EXPECT_EQ(fixture.requester.requests[1].coordinate, (Coordinate{1, 0, 0}));
}

TEST(WorldStreamingSystemTest, CompletedImmediatelyDoesNotConsumeEntireFrameBudget)
{
	spk::EngineHeadless engine;
	spk::ChunkCollection<TestChunk> collection;
	collection.attach(engine);
	ImmediateRequester requester;
	spk::Entity3D player("player");
	addObserver(engine, player, {0.0f, 0.0f, 0.0f}, {2, 0, 0});

	auto &system = engine.addSystem<Streamer>(collection, requester, Streamer::Settings{.maxInFlight = 1});
	spk::UpdateContext context{.time = 0ms, .deltaTime = 1ms};
	engine.updateState(context);

	EXPECT_EQ(requester.requested.size(), 5u);
	EXPECT_EQ(collection.size(), 5u);
	EXPECT_EQ(system.inFlightCount(), 0u);
	engine.removeSystem(system);
	collection.detach();
}

TEST(WorldStreamingSystemTest, RemovingStreamingSystemDoesNotWaitForWorkerResults)
{
	Fixture fixture;
	spk::Entity3D player("player");
	addObserver(fixture.engine, player, {0.0f, 0.0f, 0.0f});
	Streamer &streamer = fixture.start();
	fixture.tick(0ms);
	auto task = fixture.requester.requests.at(0).task;
	std::promise<void> release;
	auto gate = release.get_future().share();
	std::thread worker([task, gate]() {
		gate.wait();
		task->validate(Result(TestChunk(47)));
	});

	fixture.engine.removeSystem(streamer);
	EXPECT_TRUE(fixture.collection.empty());
	release.set_value();
	worker.join();
	EXPECT_TRUE(fixture.collection.empty());
}

TEST(WorldStreamingSystemTest, CollectionEngineRebindingDetachesSystemUntilExplicitReattachment)
{
	Fixture fixture;
	spk::Entity3D player("player");
	addObserver(fixture.engine, player, {0.0f, 0.0f, 0.0f});
	Streamer &streamer = fixture.start();
	fixture.tick(0ms);

	spk::EngineHeadless other;
	fixture.collection.attach(other);
	EXPECT_EQ(streamer.engine(), nullptr);
	EXPECT_THROW(streamer.attach(&fixture.engine), spk::Exception);
	fixture.tick(1ms);
	EXPECT_EQ(fixture.requester.requests.size(), 1u);

	fixture.collection.attach(fixture.engine);
	streamer.attach(&fixture.engine);
	fixture.tick(2ms);
	EXPECT_EQ(streamer.engine(), &fixture.engine);
	ASSERT_EQ(fixture.requester.requests.size(), 2u);
	EXPECT_EQ(fixture.requester.requests[1].coordinate, (Coordinate{0, 0, 0}));
}

TEST(WorldStreamingSystemTest, RejectsInvalidSettingsAndImplicitEngineSwitches)
{
	Fixture fixture;
	EXPECT_THROW((void)Streamer(fixture.collection, fixture.requester, {.maxInFlight = 0}), spk::Exception);
	EXPECT_THROW((void)Streamer(fixture.collection, fixture.requester, {.timeout = 0ms}), spk::Exception);
	EXPECT_THROW((void)Streamer(fixture.collection, fixture.requester, {.retryDelay = 0ms}), spk::Exception);
	Streamer &streamer = fixture.start();

	spk::EngineHeadless other;
	EXPECT_THROW(streamer.attach(&other), spk::Exception);
	EXPECT_EQ(streamer.engine(), &fixture.engine);
}
