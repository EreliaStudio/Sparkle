#include <gtest/gtest.h>

#include <exception>
#include <map>
#include <mutex>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "world/chunk.hpp"
#include "world/chunk_collection.hpp"
#include "world/generator.hpp"

namespace
{
	using Coordinate = spk::World::ChunkCoordinate;

	class ExampleChunk : public spk::World::Chunk<32, 16, 32>
	{
	private:
		int _value;
		std::vector<int> _appliedPasses;

	public:
		explicit ExampleChunk(int value, std::vector<int> appliedPasses) :
			_value(value),
			_appliedPasses(std::move(appliedPasses))
		{
		}

		[[nodiscard]] int value() const noexcept
		{
			return _value;
		}

		[[nodiscard]] const std::vector<int> &appliedPasses() const noexcept
		{
			return _appliedPasses;
		}
	};

	static_assert(!std::is_default_constructible_v<ExampleChunk>);

	struct GenerationData
	{
		void flush()
		{
			++flushCalls;
		}

		int flushCalls = 0;

		const int &seed;
		std::vector<int> *trace;
		struct ChunkData
		{
			int scratch = 0;
			int value = 0;
			std::vector<int> appliedPasses;
			std::unique_ptr<int> moveOnly = std::make_unique<int>(0);
		};
		std::mutex mutex;
		std::map<Coordinate, ChunkData> chunks;

		ChunkData &chunk(const Coordinate &coordinate)
		{
			const std::scoped_lock lock(mutex);
			return chunks.at(coordinate);
		}
		int completed = 0;

		GenerationData(const int &seedValue, std::vector<int> *traceValue) :
			seed(seedValue),
			trace(traceValue)
		{
		}
	};

	static_assert(!std::is_copy_constructible_v<GenerationData>);
	static_assert(spk::World::GenerationData<GenerationData>);
	static_assert(!spk::World::GenerationData<int>);
	struct WrongFlush
	{
		int flush();
	};
	static_assert(!spk::World::GenerationData<WrongFlush>);
	using BaseGenerator = spk::World::Generator<ExampleChunk, GenerationData>;

	class ExampleGenerator : public BaseGenerator
	{
	private:
		bool _prepareGenerationData(const Coordinate &coordinate, GenerationData &data) override
		{
			++dataCalls;
			lastCoordinate = coordinate;
			if (throwOnData)
			{
				throw std::runtime_error("generation data failed");
			}
			if (absent)
			{
				return false;
			}
			const std::scoped_lock lock(data.mutex);
			if (!data.chunks.try_emplace(coordinate).second)
			{
				throw spk::Exception("Chunk generation is already active");
			}
			return true;
		}

		ExampleChunk _instantiateChunk(const Coordinate &coordinate, GenerationData &data) override
		{
			++instantiationCalls;
			if (throwOnBuild)
			{
				throw spk::Exception("Chunk construction failed");
			}
			auto &chunk = data.chunk(coordinate);
			++data.completed;
			return ExampleChunk(chunk.value, std::move(chunk.appliedPasses));
		}

		void _cleanGenerationData(const Coordinate &coordinate, GenerationData &data) noexcept override
		{
			++cleanupCalls;
			const std::scoped_lock lock(data.mutex);
			data.chunks.erase(coordinate);
			completed = data.completed;
			remainingChunks = data.chunks.size();
		}

	public:
		int completed = 0;
		std::size_t remainingChunks = 0;
		int cleanupCalls = 0;
		int instantiationCalls = 0;
		Coordinate lastCoordinate{};
		int dataCalls = 0;
		bool absent = false;
		bool throwOnData = false;
		bool throwOnBuild = false;

		explicit ExampleGenerator(const int &seed, std::vector<int> *trace = nullptr) :
			BaseGenerator(seed, trace)
		{
		}
	};

	class RecordingPass : public BaseGenerator::Pass
	{
	private:
		int _number;
		std::vector<Coordinate> &_coordinates;
		std::vector<int> *_initialScratch;

	public:
		RecordingPass(int number, std::vector<Coordinate> &coordinates, std::vector<int> *initialScratch = nullptr) :
			_number(number),
			_coordinates(coordinates),
			_initialScratch(initialScratch)
		{
		}

		void apply(const Coordinate &coordinate, GenerationData &data) override
		{
			auto &chunk = data.chunk(coordinate);
			_coordinates.push_back(coordinate);
			if (_initialScratch != nullptr)
			{
				_initialScratch->push_back(chunk.scratch);
			}
			chunk.scratch = chunk.scratch * 10 + _number;
			chunk.value += chunk.scratch + data.seed + coordinate.x - coordinate.y + coordinate.z;
			chunk.appliedPasses.push_back(_number);
			if (data.trace != nullptr)
			{
				data.trace->push_back(_number);
			}
		}
	};

	class ThrowingPass : public BaseGenerator::Pass
	{
	public:
		enum class Failure
		{
			Standard,
			Sparkle
		};

		Failure failure;
		int &calls;

		ThrowingPass(Failure failure, int &calls) :
			failure(failure),
			calls(calls)
		{
		}

		void apply(const Coordinate &, GenerationData &) override
		{
			++calls;
			if (failure == Failure::Sparkle)
			{
				throw spk::Exception("pass rejected generation");
			}
			throw std::runtime_error("pass execution failed");
		}
	};

	class CountingPass : public BaseGenerator::Pass
	{
	public:
		int &calls;

		explicit CountingPass(int &calls) :
			calls(calls)
		{
		}

		void apply(const Coordinate &, GenerationData &) override
		{
			++calls;
		}
	};

	class LifetimePass : public BaseGenerator::Pass
	{
	private:
		int &_destructions;

	public:
		explicit LifetimePass(int &destructions) :
			_destructions(destructions)
		{
		}

		~LifetimePass() override
		{
			++_destructions;
		}

		void apply(const Coordinate &, GenerationData &) override
		{
		}
	};

	class CollectionOnlyChunk : public spk::World::Chunk<3, 5, 7>
	{
	private:
		int _value;

	public:
		explicit CollectionOnlyChunk(int value) :
			_value(value)
		{
		}

		[[nodiscard]] int value() const noexcept
		{
			return _value;
		}
	};
}

TEST(GeneratorTest, GeneratesChunkByValueAndSharesScratchDataBetweenPasses)
{
	const int seed = 10;
	std::vector<int> order;
	std::vector<Coordinate> coordinates;
	ExampleGenerator generator(seed, &order);
	generator.addPass<RecordingPass>(1, coordinates);
	generator.addPass<RecordingPass>(2, coordinates);
	generator.addPass<RecordingPass>(3, coordinates);
	const Coordinate requested{2, -1, 4};

	std::optional<ExampleChunk> result = generator.generate(requested);

	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(result->value(), 187);
	EXPECT_EQ(result->appliedPasses(), (std::vector<int>{1, 2, 3}));
	EXPECT_EQ(order, (std::vector<int>{1, 2, 3}));
	EXPECT_EQ(coordinates, (std::vector<Coordinate>{requested, requested, requested}));
	EXPECT_EQ(generator.lastCoordinate, requested);
	EXPECT_EQ(generator.instantiationCalls, 1);
	EXPECT_EQ(generator.dataCalls, 1);
}

TEST(GeneratorTest, PersistentGenerationDataHasFreshRequestScratch)
{
	const int seed = 7;
	std::vector<Coordinate> coordinates;
	std::vector<int> initialScratch;
	ExampleGenerator generator(seed);
	generator.addPass<RecordingPass>(1, coordinates, &initialScratch);
	const Coordinate first{1, 0, 0};
	const Coordinate second{-3, 2, 1};

	const auto firstResult = generator.generate(first);
	const auto secondResult = generator.generate(second);
	const auto repeatedFirst = generator.generate(first);

	ASSERT_TRUE(firstResult.has_value());
	ASSERT_TRUE(secondResult.has_value());
	ASSERT_TRUE(repeatedFirst.has_value());
	EXPECT_EQ(initialScratch, (std::vector<int>{0, 0, 0}));
	EXPECT_EQ(coordinates, (std::vector<Coordinate>{first, second, first}));
	EXPECT_EQ(firstResult->value(), repeatedFirst->value());
	EXPECT_NE(firstResult->value(), secondResult->value());
	EXPECT_EQ(generator.instantiationCalls, 3);
	EXPECT_EQ(generator.dataCalls, 3);
	EXPECT_EQ(generator.cleanupCalls, 3);
	EXPECT_EQ(generator.completed, 3);
	EXPECT_EQ(generator.remainingChunks, 0u);
}

TEST(GeneratorTest, AbsentChunkSkipsAllPassesAndInstantiation)
{
	const int seed = 1;
	ExampleGenerator generator(seed);
	generator.absent = true;
	int calls = 0;
	generator.addPass<CountingPass>(calls);

	EXPECT_FALSE(generator.generate({9, 8, 7}).has_value());
	EXPECT_EQ(generator.instantiationCalls, 0);
	EXPECT_EQ(generator.dataCalls, 1);
	EXPECT_EQ(generator.cleanupCalls, 0);
	EXPECT_EQ(calls, 0);
}

TEST(GeneratorTest, CanGenerateWithoutPasses)
{
	const int seed = 1;
	ExampleGenerator generator(seed);

	const auto result = generator.generate({-1, -2, -3});

	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(result->value(), 0);
	EXPECT_TRUE(result->appliedPasses().empty());
	EXPECT_EQ(generator.dataCalls, 1);
}

TEST(GeneratorTest, DataPreparationFailureIsAnExceptionAndSkipsPasses)
{
	const int seed = 1;
	ExampleGenerator generator(seed);
	generator.throwOnData = true;
	int calls = 0;
	generator.addPass<CountingPass>(calls);

	EXPECT_THROW((void)generator.generate({0, 0, 0}), spk::Exception);
	EXPECT_EQ(generator.instantiationCalls, 0);
	EXPECT_EQ(generator.dataCalls, 1);
	EXPECT_EQ(calls, 0);
}

TEST(GeneratorTest, StandardPassFailureIsWrappedWithCauseAndStopsLaterPasses)
{
	const int seed = 1;
	ExampleGenerator generator(seed);
	int calls = 0;
	int laterCalls = 0;
	generator.addPass<ThrowingPass>(ThrowingPass::Failure::Standard, calls);
	generator.addPass<CountingPass>(laterCalls);

	try
	{
		(void)generator.generate({0, 0, 0});
		FAIL() << "Expected an exception";
	} catch (const spk::Exception &error)
	{
		ASSERT_NE(error.cause(), nullptr);
		EXPECT_THROW(std::rethrow_exception(error.cause()), std::runtime_error);
	}
	EXPECT_EQ(calls, 1);
	EXPECT_EQ(laterCalls, 0);
	EXPECT_EQ(generator.instantiationCalls, 0);
	EXPECT_EQ(generator.cleanupCalls, 1);
	EXPECT_EQ(generator.remainingChunks, 0u);
}

TEST(GeneratorTest, SparklePassFailurePreservesOriginalException)
{
	const int seed = 1;
	ExampleGenerator generator(seed);
	int calls = 0;
	generator.addPass<ThrowingPass>(ThrowingPass::Failure::Sparkle, calls);

	try
	{
		(void)generator.generate({0, 0, 0});
		FAIL() << "Expected an exception";
	} catch (const spk::Exception &error)
	{
		EXPECT_EQ(error.message(), "pass rejected generation");
	}
	EXPECT_EQ(calls, 1);
}

TEST(GeneratorTest, FinalConstructionFailureIsAnException)
{
	const int seed = 1;
	std::vector<Coordinate> coordinates;
	ExampleGenerator generator(seed);
	generator.throwOnBuild = true;
	generator.addPass<RecordingPass>(1, coordinates);

	EXPECT_THROW((void)generator.generate({1, 2, 3}), spk::Exception);
	EXPECT_EQ(coordinates, (std::vector<Coordinate>{{1, 2, 3}}));
	EXPECT_EQ(generator.instantiationCalls, 1);
	EXPECT_EQ(generator.dataCalls, 1);
	EXPECT_EQ(generator.cleanupCalls, 1);
	EXPECT_EQ(generator.remainingChunks, 0u);
}

TEST(GeneratorTest, ResultsRemainDeterministicRegardlessOfRequestOrder)
{
	const int seed = 19;
	std::vector<Coordinate> coordinates;
	ExampleGenerator generator(seed);
	generator.addPass<RecordingPass>(1, coordinates);
	generator.addPass<RecordingPass>(2, coordinates);
	const std::vector<Coordinate> requests{{-4, 2, 0}, {3, -7, 2}, {0, 0, 0}, {8, 1, -5}};
	std::map<Coordinate, int> expected;

	for (const Coordinate &coordinate : requests)
	{
		const auto result = generator.generate(coordinate);
		ASSERT_TRUE(result.has_value());
		expected.emplace(coordinate, result->value());
	}

	for (auto it = requests.rbegin(); it != requests.rend(); ++it)
	{
		const auto result = generator.generate(*it);
		ASSERT_TRUE(result.has_value());
		EXPECT_EQ(result->value(), expected.at(*it));
		EXPECT_EQ(result->appliedPasses(), (std::vector<int>{1, 2}));
	}
}

TEST(GeneratorTest, PassesAreOwned)
{
	const int seed = 0;
	int destructions = 0;
	{
		ExampleGenerator generator(seed);
		LifetimePass &pass = generator.addPass<LifetimePass>(destructions);
		(void)pass;
		EXPECT_EQ(destructions, 0);
	}
	EXPECT_EQ(destructions, 1);
}

TEST(GeneratorTest, ChunkCollectionSupportsChunkTypesWithoutBuilder)
{
	spk::World::ChunkCollection<CollectionOnlyChunk> collection;
	const Coordinate coordinate{-2, 0, 3};
	collection.addChunk(coordinate, CollectionOnlyChunk(42));

	CollectionOnlyChunk *chunk = nullptr;
	ASSERT_TRUE(collection.tryGet(coordinate, chunk));
	ASSERT_NE(chunk, nullptr);
	EXPECT_EQ(chunk->value(), 42);
}
