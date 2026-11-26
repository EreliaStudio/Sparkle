#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <exception>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include "exception.hpp"
#include "threading/task.hpp"
#include "threading/worker_pool.hpp"
#include "world/chunk.hpp"
#include "world/chunk_collection.hpp"
#include "world/chunk_requester.hpp"
#include "world/generator.hpp"
#include "world/local_chunk_requester.hpp"

namespace
{
	using Coordinate = spk::World::ChunkCoordinate;

	class TestChunk : public spk::World::Chunk<8, 4, 6>
	{
	public:
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

	struct GenerationData
	{
		void flush()
		{
		}

		int seed = 0;
		std::mutex mutex;
		std::map<Coordinate, int> values;

		int &value(const Coordinate &coordinate)
		{
			const std::scoped_lock lock(mutex);
			return values.at(coordinate);
		}

		explicit GenerationData(int seedValue) :
			seed(seedValue)
		{
		}
	};

	using Generator = spk::World::Generator<TestChunk, GenerationData>;
	using Result = std::optional<TestChunk>;
	using Task = spk::Task<Result>;
	using Answer = Task::Answer;
	using Status = Task::Status;

	class TestGenerator : public Generator
	{
	private:
		std::atomic<int> _dataCalls = 0;

		[[nodiscard]] bool _prepareGenerationData(const Coordinate &coordinate, GenerationData &data) override
		{
			_dataCalls.fetch_add(1, std::memory_order_relaxed);
			if (coordinate.x == -2)
			{
				throw spk::Exception("Generation data could not be prepared");
			}
			if (coordinate.x == -1)
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

		[[nodiscard]] TestChunk _instantiateChunk(const Coordinate &coordinate, GenerationData &data) override
		{
			return TestChunk(data.value(coordinate));
		}

		void _cleanGenerationData(const Coordinate &coordinate, GenerationData &data) noexcept override
		{
			const std::scoped_lock lock(data.mutex);
			data.values.erase(coordinate);
		}

	public:
		explicit TestGenerator(int seed) :
			Generator(seed)
		{
		}

		[[nodiscard]] int dataCalls() const noexcept
		{
			return _dataCalls.load(std::memory_order_relaxed);
		}
	};

	class FillPass : public Generator::Pass
	{
	public:
		void apply(const Coordinate &coordinate, GenerationData &data) override
		{
			if (coordinate.x == 98)
			{
				throw spk::Exception("Expected pass rejection");
			}
			if (coordinate.x == 99)
			{
				throw std::runtime_error("Expected standard exception");
			}
			data.value(coordinate) = data.seed + coordinate.x + coordinate.y + coordinate.z;
		}
	};

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
			data.value(coordinate) = data.seed;
		}
	};

	class RemoteRequester : public spk::World::ChunkRequester<TestChunk>
	{
	public:
		[[nodiscard]] Answer request(const Coordinate &coordinate) override
		{
			Task task;
			if (coordinate.x == 0)
			{
				task.validate(std::nullopt);
			}
			else if (coordinate.x < 0)
			{
				task.fail(std::make_exception_ptr(std::runtime_error("Remote source unavailable")));
			}
			else
			{
				task.validate(Result(TestChunk(coordinate.x)));
			}
			return task.answer();
		}
	};

	[[nodiscard]] bool waitUntilSettled(const Answer &answer)
	{
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
		while (answer.status() == Status::Pending && std::chrono::steady_clock::now() < deadline)
		{
			std::this_thread::yield();
		}
		return answer.status() != Status::Pending;
	}
}

TEST(ChunkRequesterTest, LocalSourceGeneratesAChunkOnAWorker)
{
	TestGenerator generator(40);
	generator.addPass<FillPass>();
	spk::WorkerPool workers(1u);
	spk::World::LocalChunkRequester<TestChunk, GenerationData> requester(workers, generator);
	const Coordinate coordinate{2, -3, 4};

	Answer answer = requester.request(coordinate);

	ASSERT_TRUE(waitUntilSettled(answer));
	ASSERT_EQ(answer.status(), Status::Completed);
	ASSERT_TRUE(answer.result().has_value());
	EXPECT_EQ(answer.result()->value(), 43);
	EXPECT_EQ(generator.dataCalls(), 1);
}

TEST(ChunkRequesterTest, AbsentChunkIsSuccessful)
{
	TestGenerator generator(40);
	generator.addPass<FillPass>();
	spk::WorkerPool workers(1u);
	spk::World::LocalChunkRequester<TestChunk, GenerationData> requester(workers, generator);

	const Answer answer = requester.request({-1, 0, 0});

	ASSERT_TRUE(waitUntilSettled(answer));
	EXPECT_EQ(answer.status(), Status::Completed);
	EXPECT_FALSE(answer.result().has_value());
	EXPECT_EQ(generator.dataCalls(), 1);
}

TEST(ChunkRequesterTest, SparkleGenerationExceptionBecomesFailedTask)
{
	TestGenerator generator(8);
	generator.addPass<FillPass>();
	spk::WorkerPool workers(1u);
	spk::World::LocalChunkRequester<TestChunk, GenerationData> requester(workers, generator);

	const Answer answer = requester.request({-2, 0, 0});

	ASSERT_TRUE(waitUntilSettled(answer));
	EXPECT_EQ(answer.status(), Status::Failed);
	EXPECT_THROW((void)answer.get(), spk::Exception);
	try
	{
		std::rethrow_exception(answer.failure());
		FAIL() << "Expected generation exception";
	} catch (const spk::Exception &exception)
	{
		EXPECT_EQ(exception.message(), "Generation data could not be prepared");
	}
	EXPECT_EQ(generator.dataCalls(), 1);
}

TEST(ChunkRequesterTest, StandardPassExceptionIsStoredAsFailureWithOriginalCause)
{
	TestGenerator generator(8);
	generator.addPass<FillPass>();
	spk::WorkerPool workers(1u);
	spk::World::LocalChunkRequester<TestChunk, GenerationData> requester(workers, generator);

	const Answer answer = requester.request({99, 0, 0});

	ASSERT_TRUE(waitUntilSettled(answer));
	ASSERT_EQ(answer.status(), Status::Failed);
	try
	{
		std::rethrow_exception(answer.failure());
		FAIL() << "Expected a wrapped generation failure";
	} catch (const spk::Exception &exception)
	{
		ASSERT_NE(exception.cause(), nullptr);
		EXPECT_THROW(std::rethrow_exception(exception.cause()), std::runtime_error);
	}
}

TEST(ChunkRequesterTest, PassFailureDoesNotStopSubsequentRequests)
{
	TestGenerator generator(5);
	generator.addPass<FillPass>();
	spk::WorkerPool workers(1u);
	spk::World::LocalChunkRequester<TestChunk, GenerationData> requester(workers, generator);

	const Answer failed = requester.request({98, 0, 0});
	const Answer succeeded = requester.request({3, 4, 5});

	ASSERT_TRUE(waitUntilSettled(failed));
	ASSERT_TRUE(waitUntilSettled(succeeded));
	EXPECT_EQ(failed.status(), Status::Failed);
	ASSERT_EQ(succeeded.status(), Status::Completed);
	ASSERT_TRUE(succeeded.result().has_value());
	EXPECT_EQ(succeeded.result()->value(), 17);
}

TEST(ChunkRequesterTest, CopiesCompletedValueIntoCollectionOnlyOnCallingThread)
{
	TestGenerator generator(6);
	generator.addPass<FillPass>();
	spk::WorkerPool workers(1u);
	spk::World::LocalChunkRequester<TestChunk, GenerationData> requester(workers, generator);
	spk::World::ChunkCollection<TestChunk> collection;
	const Coordinate coordinate{1, 2, 3};

	const Answer answer = requester.request(coordinate);
	ASSERT_TRUE(waitUntilSettled(answer));
	ASSERT_EQ(answer.status(), Status::Completed);
	EXPECT_FALSE(collection.contains(coordinate));

	const Result &result = answer.result();
	ASSERT_TRUE(result.has_value());
	collection.addChunk(coordinate, *result);

	ASSERT_TRUE(collection.contains(coordinate));
	EXPECT_EQ(collection.at(coordinate).value(), 12);
	EXPECT_NE(&collection.at(coordinate), &result.value());
}

TEST(ChunkRequesterTest, CompletedTaskCallbackIsInvokedImmediately)
{
	TestGenerator generator(1);
	generator.addPass<FillPass>();
	spk::WorkerPool workers(1u);
	spk::World::LocalChunkRequester<TestChunk, GenerationData> requester(workers, generator);

	const Answer answer = requester.request({1, 0, 0});
	ASSERT_TRUE(waitUntilSettled(answer));

	int calls = 0;
	auto contract = answer.subscribeToCompletion([&]() {
		++calls;
		EXPECT_EQ(answer.status(), Status::Completed);
	});

	EXPECT_EQ(calls, 1);
	EXPECT_FALSE(contract.isValid());
	EXPECT_EQ(answer.result()->value(), 2);
}

TEST(ChunkRequesterTest, RemoteSourceUsesSameInterfaceWithoutGeneratorOrPool)
{
	std::unique_ptr<spk::World::ChunkRequester<TestChunk>> requester = std::make_unique<RemoteRequester>();
	const Answer available = requester->request({51, 0, 0});
	const Answer absent = requester->request({0, 0, 0});
	const Answer failed = requester->request({-3, 0, 0});

	ASSERT_EQ(available.status(), Status::Completed);
	ASSERT_TRUE(available.result().has_value());
	EXPECT_EQ(available.result()->value(), 51);
	EXPECT_EQ(absent.status(), Status::Completed);
	EXPECT_FALSE(absent.result().has_value());
	EXPECT_EQ(failed.status(), Status::Failed);
	EXPECT_THROW(std::rethrow_exception(failed.failure()), std::runtime_error);
}

TEST(ChunkRequesterTest, InFlightTaskDoesNotDependOnRequesterLifetime)
{
	std::promise<void> entered;
	auto enteredFuture = entered.get_future();
	std::promise<void> release;
	auto gate = release.get_future().share();
	TestGenerator generator(73);
	generator.addPass<BlockingPass>(entered, gate);
	spk::WorkerPool workers(1u);
	std::optional<Answer> answer;
	{
		spk::World::LocalChunkRequester<TestChunk, GenerationData> requester(workers, generator);
		answer.emplace(requester.request({0, 0, 0}));
	}

	const auto enteredStatus = enteredFuture.wait_for(std::chrono::seconds(5));
	EXPECT_EQ(enteredStatus, std::future_status::ready);
	EXPECT_EQ(answer->status(), Status::Pending);
	release.set_value();

	ASSERT_TRUE(waitUntilSettled(*answer));
	ASSERT_EQ(answer->status(), Status::Completed);
	EXPECT_EQ(answer->result()->value(), 73);
}

TEST(ChunkRequesterTest, ResignedInFlightSubscriptionDoesNotCallDestroyedConsumer)
{
	std::promise<void> entered;
	auto enteredFuture = entered.get_future();
	std::promise<void> release;
	auto gate = release.get_future().share();
	TestGenerator generator(27);
	generator.addPass<BlockingPass>(entered, gate);
	spk::WorkerPool workers(1u);
	spk::World::LocalChunkRequester<TestChunk, GenerationData> requester(workers, generator);

	const Answer answer = requester.request({0, 0, 0});
	auto consumerAlive = std::make_shared<std::atomic<bool>>(true);
	std::atomic<int> notifications = 0;
	auto contract = answer.subscribeToCompletion([&]() {
		if (consumerAlive->load(std::memory_order_relaxed))
		{
			notifications.fetch_add(1, std::memory_order_relaxed);
		}
	});

	EXPECT_EQ(enteredFuture.wait_for(std::chrono::seconds(5)), std::future_status::ready);
	contract.resign();
	consumerAlive->store(false, std::memory_order_relaxed);
	release.set_value();

	ASSERT_TRUE(waitUntilSettled(answer));
	EXPECT_FALSE(contract.isValid());
	EXPECT_EQ(notifications.load(std::memory_order_relaxed), 0);
	EXPECT_EQ(answer.result()->value(), 27);
}

TEST(ChunkRequesterTest, IndependentRequestsKeepTheirOwnCoordinates)
{
	TestGenerator generator(30);
	generator.addPass<FillPass>();
	spk::WorkerPool workers(4u);
	spk::World::LocalChunkRequester<TestChunk, GenerationData> requester(workers, generator);
	const std::vector<Coordinate> coordinates{{0, 0, 0}, {3, -7, 2}, {7, 5, -8}, {2, 0, 4}};
	std::vector<Answer> answers;

	for (const Coordinate &coordinate : coordinates)
	{
		answers.push_back(requester.request(coordinate));
	}
	for (std::size_t index = 0; index < coordinates.size(); ++index)
	{
		const Answer &answer = answers[index];
		ASSERT_TRUE(waitUntilSettled(answer));
		ASSERT_EQ(answer.status(), Status::Completed);
		ASSERT_TRUE(answer.result().has_value());
		const Coordinate &coordinate = coordinates[index];
		EXPECT_EQ(answer.result()->value(), 30 + coordinate.x + coordinate.y + coordinate.z);
	}
	EXPECT_EQ(generator.dataCalls(), static_cast<int>(coordinates.size()));
}
