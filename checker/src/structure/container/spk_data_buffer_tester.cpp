#include "structure/container/spk_data_buffer_tester.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace
{
	struct SampleStruct
	{
		int32_t id;
		float value;
	};
}

TEST_F(DataBufferTest, DefaultState)
{
	EXPECT_EQ(buffer.size(), 0U) << "A default constructed buffer should have a size of 0";
	EXPECT_EQ(buffer.bookmark(), 0U) << "A default constructed buffer should start with a bookmark at 0";
	EXPECT_EQ(buffer.leftover(), 0U) << "A default constructed buffer should not report any leftover bytes";
	EXPECT_EQ(buffer.empty(), true) << "A default constructed buffer should be reported as empty";
}

TEST_F(DataBufferTest, ConstructorsAndAssignmentsPreserveContent)
{
	spk::DataBuffer original;
	std::vector<uint32_t> values = {10U, 20U, 30U};

	for (const auto value : values)
	{
		original << value;
	}

	spk::DataBuffer copy(original);
	EXPECT_EQ(copy.size(), original.size()) << "Copy constructor should duplicate the buffer size";
	EXPECT_EQ(copy.bookmark(), original.bookmark()) << "Copy constructor should preserve the bookmark value";

	std::vector<uint32_t> copyExtracted(values.size());
	for (size_t i = 0; i < copyExtracted.size(); ++i)
	{
		copy >> copyExtracted[i];
		EXPECT_EQ(copyExtracted[i], values[i]) << "Copy should preserve element " << i;
	}

	original.skip(sizeof(uint32_t));
	spk::DataBuffer assigned;
	assigned = original;
	EXPECT_EQ(assigned.bookmark(), original.bookmark()) << "Copy assignment should preserve the bookmark";
	assigned.reset();

	std::vector<uint32_t> assignedExtracted(values.size());
	for (size_t i = 0; i < assignedExtracted.size(); ++i)
	{
		assigned >> assignedExtracted[i];
		EXPECT_EQ(assignedExtracted[i], values[i]) << "Copy assignment should preserve value at index " << i;
	}

	spk::DataBuffer moved(std::move(original));
	EXPECT_EQ(moved.size(), sizeof(uint32_t) * values.size()) << "Move constructor should keep the original payload size";
	EXPECT_EQ(moved.bookmark(), sizeof(uint32_t)) << "Move constructor should keep the bookmark value";
	moved.reset();

	std::vector<uint32_t> movedExtracted(values.size());
	for (size_t i = 0; i < movedExtracted.size(); ++i)
	{
		moved >> movedExtracted[i];
		EXPECT_EQ(movedExtracted[i], values[i]) << "Move construction should preserve value at index " << i;
	}

	spk::DataBuffer moveSource;
	moveSource << static_cast<uint16_t>(512U);
	spk::DataBuffer moveAssigned;
	moveAssigned = std::move(moveSource);
	EXPECT_EQ(moveAssigned.size(), sizeof(uint16_t)) << "Move assignment should transfer the payload size";
	EXPECT_EQ(moveAssigned.bookmark(), 0U) << "Move assignment should reset the bookmark to the source bookmark";
	uint16_t moveAssignedValue = moveAssigned.get<uint16_t>();
	EXPECT_EQ(moveAssignedValue, 512U) << "Move assignment should transfer the stored value";
}

TEST_F(DataBufferTest, ResizeAndClearModifySizeAndBookmark)
{
	buffer.resize(64U);
	EXPECT_EQ(buffer.size(), 64U) << "Resizing should change the buffer size";
	EXPECT_EQ(buffer.leftover(), 64U) << "After resizing the leftover bytes should match the new size";

	buffer.skip(16U);
	EXPECT_EQ(buffer.bookmark(), 16U) << "Skipping should advance the bookmark";
	EXPECT_EQ(buffer.leftover(), 48U) << "Skipping should decrease the leftover byte count";

	buffer.clear();
	EXPECT_EQ(buffer.size(), 0U) << "Clearing should reset the size to 0";
	EXPECT_EQ(buffer.bookmark(), 0U) << "Clearing should reset the bookmark to 0";
	EXPECT_EQ(buffer.empty(), true) << "Clearing should result in an empty buffer";
}

TEST_F(DataBufferTest, SkipAndResetHandleBounds)
{
	buffer.resize(32U);
	EXPECT_NO_THROW(buffer.skip(32U)) << "Skipping exactly the leftover bytes should be allowed";
	EXPECT_EQ(buffer.bookmark(), 32U) << "Skipping the full buffer should leave the bookmark at the end";
	EXPECT_THROW(buffer.skip(1U), std::runtime_error) << "Skipping beyond the buffer should throw";

	buffer.reset();
	EXPECT_EQ(buffer.bookmark(), 0U) << "Reset should return the bookmark to the beginning";
}

TEST_F(DataBufferTest, PushAndPullRawData)
{
	std::array<uint8_t, 4> data = {0x10U, 0x20U, 0x30U, 0x40U};
	buffer.push(data.data(), data.size());
	EXPECT_EQ(buffer.size(), data.size()) << "Push should append bytes to the buffer";
	EXPECT_EQ(buffer.leftover(), data.size()) << "After pushing the leftover bytes should match the pushed data size";

	std::array<uint8_t, 4> output = {0U, 0U, 0U, 0U};
	buffer.pull(output.data(), output.size());
	for (size_t i = 0; i < output.size(); ++i)
	{
		EXPECT_EQ(output[i], data[i]) << "Pulled byte at index " << i << " should match the pushed data";
	}
	EXPECT_EQ(buffer.leftover(), 0U) << "After pulling all data there should be no leftover bytes";
}

TEST_F(DataBufferTest, PullThrowsWhenNotEnoughData)
{
	buffer.resize(2U);
	std::array<uint8_t, 4> output = {0U, 0U, 0U, 0U};
	EXPECT_THROW(buffer.pull(output.data(), output.size()), std::runtime_error) << "Pulling more data than available should throw";
}

TEST_F(DataBufferTest, AppendAndEditRawData)
{
	std::string text = "sparkle";
	buffer.append(text.data(), text.size());
	EXPECT_EQ(buffer.size(), text.size()) << "Append should increase the buffer size by the appended amount";

	const std::string replacement = "shine";
	buffer.resize(16U);
	buffer.edit(0U, replacement.data(), replacement.size());
	EXPECT_EQ(std::string(reinterpret_cast<const char *>(buffer.data()), replacement.size()), replacement)
		<< "Editing raw data should replace the stored bytes";

	SampleStruct sample{42, 3.5F};
	buffer.resize(sizeof(SampleStruct));
	buffer.edit(0U, sample);
	SampleStruct extracted = buffer.get<SampleStruct>();
	EXPECT_EQ(extracted.id, sample.id) << "Edited struct should preserve the integer field";
	EXPECT_FLOAT_EQ(extracted.value, sample.value) << "Edited struct should preserve the floating-point field";

	buffer.resize(sizeof(SampleStruct));
	EXPECT_THROW(buffer.edit(sizeof(SampleStruct), sample), std::runtime_error) << "Editing beyond the buffer size should throw an exception";
}

TEST_F(DataBufferTest, StreamOperatorsHandleStandardLayoutTypes)
{
	SampleStruct sample{7, 9.25F};
	buffer << sample;
	buffer << static_cast<int32_t>(-11);

	SampleStruct retrieved = buffer.get<SampleStruct>();
	EXPECT_EQ(retrieved.id, sample.id) << "Inserted struct should be retrieved with the same id";
	EXPECT_FLOAT_EQ(retrieved.value, sample.value) << "Inserted struct should be retrieved with the same value";

	int32_t number = buffer.get<int32_t>();
	EXPECT_EQ(number, -11) << "Inserted integer should be retrieved correctly";
	EXPECT_EQ(buffer.empty(), true) << "All inserted elements should have been consumed";
}

TEST_F(DataBufferTest, StreamOperatorsHandleContainers)
{
	std::vector<int32_t> numbers = {1, 2, 3, 4};
	std::vector<std::string> words = {"alpha", "beta", "gamma"};

	buffer << numbers;
	buffer << words;

	std::vector<int32_t> retrievedNumbers;
	buffer >> retrievedNumbers;
	EXPECT_EQ(retrievedNumbers, numbers) << "Extracted integer vector should match the inserted one";

	std::vector<std::string> retrievedWords;
	buffer >> retrievedWords;
	EXPECT_EQ(retrievedWords, words) << "Extracted string vector should match the inserted one";
	EXPECT_EQ(buffer.empty(), true) << "All container data should have been consumed";
}

TEST_F(DataBufferTest, PeekDoesNotAdvanceBookmark)
{
	std::vector<int32_t> numbers = {5, 6, 7};
	buffer << numbers;

	auto peekedNumbers = buffer.peek<std::vector<int32_t>>();
	EXPECT_EQ(peekedNumbers, numbers) << "Peek should return the full container content";
	EXPECT_EQ(buffer.bookmark(), 0U) << "Peek should not advance the bookmark";

	auto peekedValue = buffer.peek<int32_t>();
	EXPECT_EQ(peekedValue, numbers.size()) << "Peeking a scalar should return the size of the numbers vector";
	EXPECT_EQ(buffer.bookmark(), 0U) << "Peeking a scalar should not advance the bookmark";

	std::vector<int32_t> retrieved;
	buffer >> retrieved;
	EXPECT_EQ(retrieved, numbers) << "After peeking the data should still be retrievable";
}

TEST_F(DataBufferTest, PushIgnoresNullOrEmptyInput)
{
	buffer.push(nullptr, 8U);
	EXPECT_EQ(buffer.size(), 0U) << "Pushing a null pointer should have no effect";

	std::array<uint8_t, 2> values = {1U, 2U};
	buffer.push(values.data(), 0U);
	EXPECT_EQ(buffer.size(), 0U) << "Pushing zero bytes should have no effect";

	buffer.pull(nullptr, 16U);
	EXPECT_EQ(buffer.bookmark(), 0U) << "Pulling into a null pointer should leave the bookmark unchanged";
}