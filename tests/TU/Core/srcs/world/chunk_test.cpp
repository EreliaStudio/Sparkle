#include <gtest/gtest.h>

#include <memory>
#include <utility>

#include "world/chunk.hpp"

namespace
{
	class EditableChunk : public spk::World::Chunk<32, 16, 8>
	{
	private:
		int _value = 0;

	public:
		void setValue(int value)
		{
			if (_value == value)
			{
				return;
			}
			_value = value;
			notifyChunkEdition();
		}

		[[nodiscard]] int value() const noexcept
		{
			return _value;
		}
	};
}

TEST(ChunkTest, SettersNotifyOnlyAfterSuccessfulChanges)
{
	EditableChunk chunk;
	int calls = 0;
	auto contract = chunk.subscribeToChunkEdition([&]() {
		++calls;
		EXPECT_EQ(chunk.value(), calls);
	});

	chunk.setValue(1);
	chunk.setValue(1);
	chunk.setValue(2);
	EXPECT_EQ(calls, 2);
	EXPECT_TRUE(contract.isValid());
}

TEST(ChunkTest, CopyConstructingCreatesAnIndependentProvider)
{
	EditableChunk source;
	int sourceCalls = 0;
	auto sourceContract = source.subscribeToChunkEdition([&]() {
		++sourceCalls;
	});
	source.setValue(4);

	EditableChunk copy(source);
	int copyCalls = 0;
	auto copyContract = copy.subscribeToChunkEdition([&]() {
		++copyCalls;
	});

	copy.setValue(5);
	EXPECT_EQ(copy.value(), 5);
	EXPECT_EQ(source.value(), 4);
	EXPECT_EQ(sourceCalls, 1);
	EXPECT_EQ(copyCalls, 1);

	source.setValue(6);
	EXPECT_EQ(sourceCalls, 2);
	EXPECT_EQ(copyCalls, 1);
	EXPECT_TRUE(sourceContract.isValid());
	EXPECT_TRUE(copyContract.isValid());
}

TEST(ChunkTest, MoveConstructingCreatesAnIndependentProvider)
{
	EditableChunk source;
	int sourceCalls = 0;
	auto sourceContract = source.subscribeToChunkEdition([&]() {
		++sourceCalls;
	});
	source.setValue(2);

	EditableChunk moved(std::move(source));
	int movedCalls = 0;
	auto movedContract = moved.subscribeToChunkEdition([&]() {
		++movedCalls;
	});
	moved.setValue(3);
	EXPECT_EQ(moved.value(), 3);
	EXPECT_EQ(movedCalls, 1);
	EXPECT_EQ(sourceCalls, 1);
	EXPECT_TRUE(sourceContract.isValid());
	EXPECT_TRUE(movedContract.isValid());
}

TEST(ChunkTest, AssignmentsPreserveDestinationSubscriptionsWithoutNotifying)
{
	EditableChunk source;
	EditableChunk destination;
	int sourceCalls = 0;
	int destinationCalls = 0;
	auto sourceContract = source.subscribeToChunkEdition([&]() {
		++sourceCalls;
	});
	auto destinationContract = destination.subscribeToChunkEdition([&]() {
		++destinationCalls;
	});
	source.setValue(10);
	destination.setValue(1);
	EXPECT_EQ(sourceCalls, 1);
	EXPECT_EQ(destinationCalls, 1);

	destination = source;
	EXPECT_EQ(destination.value(), 10);
	EXPECT_EQ(destinationCalls, 1);
	EXPECT_TRUE(sourceContract.isValid());
	EXPECT_TRUE(destinationContract.isValid());

	destination.setValue(11);
	EXPECT_EQ(sourceCalls, 1);
	EXPECT_EQ(destinationCalls, 2);

	EditableChunk another;
	another.setValue(20);
	destination = std::move(another);
	EXPECT_EQ(destination.value(), 20);
	EXPECT_EQ(destinationCalls, 2);
	destination.setValue(21);
	EXPECT_EQ(destinationCalls, 3);
}

TEST(ChunkTest, DestructionInvalidatesOwnSubscriptions)
{
	spk::World::Chunk<1, 1, 1>::EditionContract contract;
	{
		auto chunk = std::make_unique<spk::World::Chunk<1, 1, 1>>();
		contract = chunk->subscribeToChunkEdition([]() {
		});
		EXPECT_TRUE(contract.isValid());
	}
	EXPECT_FALSE(contract.isValid());
}

TEST(ChunkTest, SubscriberCanResignDuringNotification)
{
	EditableChunk chunk;
	int firstCalls = 0;
	int secondCalls = 0;
	spk::World::Chunk<32, 16, 8>::EditionContract second;
	auto first = chunk.subscribeToChunkEdition([&]() {
		++firstCalls;
		second.resign();
	});
	second = chunk.subscribeToChunkEdition([&]() {
		++secondCalls;
	});

	chunk.setValue(1);
	chunk.setValue(2);
	EXPECT_EQ(firstCalls, 2);
	EXPECT_EQ(secondCalls, 0);
	EXPECT_FALSE(second.isValid());
}
