#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <vector>

#include "graphics/opengl/index_buffer.hpp"

TEST(BufferCPUStorageTest, AppendsAcrossStorageUnitsPreserveBytesAndResizeZeroesNewBytes)
{
	spk::IndexBuffer buffer;
	buffer.setType(spk::IndexBuffer::Type::UnsignedByte);
	std::vector<std::uint8_t> expected;
	for (std::uint8_t size = 1; size <= 65; ++size)
	{
		const std::vector<std::uint8_t> bytes(size, size);
		buffer.insert<std::uint8_t>(bytes);
		expected.insert(expected.end(), bytes.begin(), bytes.end());
		const auto actual = buffer.cast<std::uint8_t>();
		EXPECT_EQ(std::vector<std::uint8_t>(actual.begin(), actual.end()), expected);
	}

	buffer.resize<std::uint8_t>(3);
	const std::array<std::uint8_t, 3> tail{91, 92, 93};
	buffer.insert<std::uint8_t>(tail);
	expected.resize(3);
	expected.insert(expected.end(), tail.begin(), tail.end());
	buffer.resize<std::uint8_t>(67);
	expected.resize(67, 0);
	const auto actual = buffer.cast<std::uint8_t>();
	EXPECT_EQ(std::vector<std::uint8_t>(actual.begin(), actual.end()), expected);

	buffer.clear();
	buffer.resize<std::uint8_t>(67);
	for (const auto byte : buffer.cast<std::uint8_t>())
	{
		EXPECT_EQ(byte, 0);
	}
}
