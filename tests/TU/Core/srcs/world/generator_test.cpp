#include <gtest/gtest.h>

#include <exception>
#include <map>
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
	using Coordinate = spk::ChunkCoordinate;

	class ExampleChunk : public spk::Chunk<32, 16, 32>
	{
	public:
		class Builder;

	private:
		int _value;
		std::vector<int> _appliedPasses;

		explicit ExampleChunk(int value, std::vector<int> appliedPasses) :
			_value(value),
			_appliedPasses(std::move(appliedPasses))
		{
		}

		friend class Builder;

	public:
		[[nodiscard]] int value() const noexcept
		{
			return _value;
		}

		[[nodiscard]] const std::vector<int> &appliedPasses() const noexcept
		{
			return _appliedPasses;
		}
	};

	class ExampleChunk::Builder
	{
	private:
		int _value = 0;
		std::vector<int> _appliedPasses;
		bool _consumed = false;
		bool _throwOnBuild = false;

	public:
		explicit Builder(bool throwOnBuild = false) :
			_throwOnBuild(throwOnBuild)
		{
		}

		Builder(const Builder &) = delete;
		Builder &operator=(const Builder &) = delete;
		Builder(Builder &&) = default;
		Builder &operator=(Builder &&) = default;

		void add(int value)
		{
			_value += value;
		}

		void record(int pass)
		{
			_appliedPasses.push_back(pass);
		}

		void load(const ExampleChunk &chunk)
		{
			_value = chunk.value();
			_appliedPasses = chunk.appliedPasses();
		}

		[[nodiscard]] bool consumed() const noexcept
		{
			return _consumed;
		}

		[[nodiscard]] ExampleChunk build() &&
		{
			if (std::exchange(_consumed, true))
			{
				throw spk::Exception("Builder has already been consumed");
			}
			if (_throwOnBuild)
			{
				throw spk::Exception("Chunk construction failed");
			}
			return ExampleChunk(_value, std::move(_appliedPasses));
		}
	};

	static_assert(std::is_same_v<decltype(std::declval<ExampleChunk::Builder &&>().build()), ExampleChunk>);
	static_assert(!std::is_copy_constructible_v<ExampleChunk::Builder>);

	struct GenerationData
	{
		const int &seed;
		int scratch = 0;
		std::vector<int> *trace = nullptr;
	};

	using BaseGenerator = spk::Generator<ExampleChunk, GenerationData>;

	class ExampleGenerator : public BaseGenerator
	{
	private:
		const int &_seed;
		std::vector<int> *_trace;

		std::optional<ExampleChunk::Builder> _createChunkBuilder(const Coordinate &) override
		{
			++builderCalls;
			if (throwOnBuilder)
			{
				throw std::runtime_error("builder preparation failed");
			}
			if (absent)
			{
				return std::nullopt;
			}
			return std::optional<ExampleChunk::Builder>(std::in_place, throwOnBuild);
		}

		GenerationData _createGenerationData() override
		{
			++dataCalls;
			if (throwOnData)
			{
				throw std::runtime_error("generation data failed");
			}
			return {_seed, 0, _trace};
		}

	public:
		int builderCalls = 0;
		int dataCalls = 0;
		bool absent = false;
		bool throwOnBuilder = false;
		bool throwOnData = false;
		bool throwOnBuild = false;

		explicit ExampleGenerator(const int &seed, std::vector<int> *trace = nullptr) :
			_seed(seed),
			_trace(trace)
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

		void apply(ExampleChunk::Builder &builder, const Coordinate &coordinate, GenerationData &data) override
		{
			_coordinates.push_back(coordinate);
			if (_initialScratch != nullptr)
			{
				_initialScratch->push_back(data.scratch);
			}
			data.scratch = data.scratch * 10 + _number;
			builder.add(data.scratch + data.seed + coordinate.x - coordinate.y + coordinate.z);
			builder.record(_number);
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

		void apply(ExampleChunk::Builder &, const Coordinate &, GenerationData &) override
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

		void apply(ExampleChunk::Builder &, const Coordinate &, GenerationData &) override
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

		void apply(ExampleChunk::Builder &, const Coordinate &, GenerationData &) override
		{
		}
	};

	class CollectionOnlyChunk : public spk::Chunk<3, 5, 7>
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
	EXPECT_EQ(generator.builderCalls, 1);
	EXPECT_EQ(generator.dataCalls, 1);
}

TEST(GeneratorTest, EachRequestHasFreshGenerationDataAndBuilder)
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
	EXPECT_EQ(generator.builderCalls, 3);
	EXPECT_EQ(generator.dataCalls, 3);
}

TEST(GeneratorTest, AbsentChunkSkipsGenerationDataAndAllPasses)
{
	const int seed = 1;
	ExampleGenerator generator(seed);
	generator.absent = true;
	int calls = 0;
	generator.addPass<CountingPass>(calls);

	EXPECT_FALSE(generator.generate({9, 8, 7}).has_value());
	EXPECT_EQ(generator.builderCalls, 1);
	EXPECT_EQ(generator.dataCalls, 0);
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

TEST(GeneratorTest, BuilderPreparationFailureIsAnExceptionNotAnAbsentChunk)
{
	const int seed = 1;
	ExampleGenerator generator(seed);
	generator.throwOnBuilder = true;
	int calls = 0;
	generator.addPass<CountingPass>(calls);

	try
	{
		(void)generator.generate({0, 0, 0});
		FAIL() << "Expected an exception";
	} catch (const spk::Exception &error)
	{
		EXPECT_NE(error.cause(), nullptr);
	}
	EXPECT_EQ(generator.builderCalls, 1);
	EXPECT_EQ(generator.dataCalls, 0);
	EXPECT_EQ(calls, 0);
}

TEST(GeneratorTest, DataPreparationFailureIsAnExceptionAndSkipsPasses)
{
	const int seed = 1;
	ExampleGenerator generator(seed);
	generator.throwOnData = true;
	int calls = 0;
	generator.addPass<CountingPass>(calls);

	EXPECT_THROW((void)generator.generate({0, 0, 0}), spk::Exception);
	EXPECT_EQ(generator.builderCalls, 1);
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
	EXPECT_EQ(generator.dataCalls, 1);
}

TEST(GeneratorTest, BuilderBuildConsumesItsInstanceAndProducesChunkByValue)
{
	ExampleChunk::Builder builder;
	builder.add(8);
	builder.record(7);
	ExampleChunk chunk = std::move(builder).build();

	EXPECT_EQ(chunk.value(), 8);
	EXPECT_EQ(chunk.appliedPasses(), (std::vector<int>{7}));
	EXPECT_TRUE(builder.consumed());
	EXPECT_THROW((void)std::move(builder).build(), spk::Exception);
}

TEST(GeneratorTest, ConcreteBuilderCanLoadExistingChunkAndRebuildIt)
{
	ExampleChunk::Builder initial;
	initial.add(30);
	initial.record(1);
	const ExampleChunk existing = std::move(initial).build();

	ExampleChunk::Builder replacement;
	replacement.load(existing);
	replacement.add(12);
	replacement.record(2);
	ExampleChunk edited = std::move(replacement).build();

	EXPECT_EQ(existing.value(), 30);
	EXPECT_EQ(existing.appliedPasses(), (std::vector<int>{1}));
	EXPECT_EQ(edited.value(), 42);
	EXPECT_EQ(edited.appliedPasses(), (std::vector<int>{1, 2}));
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
	spk::ChunkCollection<CollectionOnlyChunk> collection;
	const Coordinate coordinate{-2, 0, 3};
	collection.addChunk(coordinate, CollectionOnlyChunk(42));

	CollectionOnlyChunk *chunk = nullptr;
	ASSERT_TRUE(collection.tryGet(coordinate, chunk));
	ASSERT_NE(chunk, nullptr);
	EXPECT_EQ(chunk->value(), 42);
}
