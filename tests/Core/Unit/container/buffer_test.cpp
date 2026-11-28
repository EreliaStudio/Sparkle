#include <gtest/gtest.h>

#include "container/buffer.hpp"

#include <array>
#include <limits>
#include <type_traits>
#include <utility>

namespace
{
	using Buffer = spk::Buffer;

	static_assert(std::is_abstract_v<Buffer>);
	static_assert(!std::is_copy_constructible_v<Buffer::Builder>);
	static_assert(std::is_move_constructible_v<Buffer::Builder>);
	static_assert(std::is_copy_constructible_v<Buffer::Immutable>);
	static_assert(std::is_copy_constructible_v<Buffer::Mutable>);
	static_assert(std::is_same_v<decltype(std::declval<Buffer::Immutable &>().cast<int>()), std::span<const int>>);
	static_assert(std::is_same_v<decltype(std::declval<Buffer::Mutable &>().cast<int>()), std::span<int>>);
	static_assert(std::is_same_v<decltype(std::declval<const Buffer::Mutable &>().cast<int>()), std::span<const int>>);
}

TEST(BufferTest, EmptyImmutableBuffer)
{
	auto buffer = Buffer::Builder().build();
	EXPECT_TRUE(buffer.empty());
	EXPECT_EQ(buffer.size(), 0u);
	EXPECT_EQ(buffer.version(), 0u);
	EXPECT_TRUE(buffer.cast<int>().empty());
}

TEST(BufferTest, BuilderPublishesWithoutCopyingPayload)
{
	Buffer::Builder builder;
	builder.reserve<int>(64);
	builder.pushBack(42);
	builder.pushBack(73);
	const auto *address = builder.data();
	auto buffer = std::move(builder).build();
	EXPECT_EQ(buffer.data(), address);
	ASSERT_EQ(buffer.cast<int>().size(), 2u);
	EXPECT_EQ(buffer.cast<int>()[0], 42);
	EXPECT_EQ(buffer.cast<int>()[1], 73);
	EXPECT_THROW(builder.resize(1), spk::Exception);
	EXPECT_THROW((void)std::move(builder).buildMutable(), spk::Exception);
}

TEST(BufferTest, BuilderCanResizeAndPopulateTypedStorage)
{
	Buffer::Builder builder;
	builder.resize<int>(3);
	auto values = builder.cast<int>();
	EXPECT_EQ(values[0], 0);
	values[1] = 17;
	auto buffer = std::move(builder).build();
	EXPECT_EQ(buffer.size(), 3 * sizeof(int));
	EXPECT_EQ(buffer.cast<int>()[1], 17);
}

TEST(BufferTest, BuilderMoveTransfersExclusiveOwnership)
{
	Buffer::Builder source;
	source.pushBack(12);
	auto *address = source.data();
	Buffer::Builder destination(std::move(source));
	EXPECT_EQ(destination.data(), address);
	EXPECT_THROW((void)source.size(), spk::Exception);
	EXPECT_EQ(std::move(destination).build().cast<int>()[0], 12);
}

TEST(BufferTest, ImmutableCopiesRetainSharedData)
{
	auto buffer = [] {
		Buffer::Builder builder;
		builder.pushBack(19);
		return std::move(builder).build();
	}();
	auto copy = buffer;
	EXPECT_EQ(copy.data(), buffer.data());
	buffer = Buffer::Builder().build();
	EXPECT_EQ(copy.cast<int>()[0], 19);
	EXPECT_EQ(copy.version(), 0u);
}

TEST(BufferTest, MutableCopiesShareContentsSizeAndVersion)
{
	Buffer::Builder builder;
	builder.pushBack(21);
	auto buffer = std::move(builder).buildMutable();
	auto copy = buffer;
	const Buffer *view = &copy;
	buffer.resize<int>(4);
	buffer.cast<int>()[1] = 99;
	buffer.validate();
	EXPECT_EQ(view->size(), 4 * sizeof(int));
	EXPECT_EQ(view->cast<int>()[0], 21);
	EXPECT_EQ(view->cast<int>()[1], 99);
	EXPECT_EQ(view->version(), 1u);
	copy.clear();
	copy.validate();
	EXPECT_TRUE(buffer.empty());
	EXPECT_EQ(buffer.version(), 2u);
}

TEST(BufferTest, InsertSupportsAppendingAnExistingView)
{
	Buffer::Builder builder;
	const std::array values{1, 2, 3};
	builder.insert(std::span(values));
	builder.insert(builder.cast<int>());
	auto buffer = std::move(builder).buildMutable();
	ASSERT_EQ(buffer.cast<int>().size(), 6u);
	EXPECT_EQ(buffer.cast<int>()[3], 1);
	buffer.pushBack(buffer.cast<int>()[0]);
	EXPECT_EQ(buffer.cast<int>().back(), 1);
}

TEST(BufferTest, RejectsInvalidTypedAccessAndOverflow)
{
	Buffer::Builder builder;
	builder.resize(3);
	EXPECT_THROW((void)builder.cast<int>(), spk::Exception);
	EXPECT_THROW(builder.resize<int>(std::numeric_limits<std::size_t>::max()), spk::Exception);
}

TEST(BufferTest, PoolReusesGrownBufferAndResetsVersion)
{
	const std::byte *address = nullptr;
	{
		Buffer::Builder builder;
		builder.reserve(1);
		auto buffer = std::move(builder).buildMutable();
		buffer.reserve(16384);
		buffer.resize(16384);
		address = buffer.data();
		buffer.validate();
	}
	Buffer::Builder builder;
	builder.reserve(16384);
	EXPECT_EQ(builder.data(), address);
	EXPECT_TRUE(builder.empty());
	EXPECT_EQ(std::move(builder).build().version(), 0u);
}

TEST(BufferTest, SupportsSixteenByteAlignedShaderDataAfterResizeAndPublication)
{
	struct alignas(16) Value
	{
		std::array<std::uint32_t, 4> elements;
	};

	Buffer::Builder builder;
	builder.pushBack(Value{{1, 2, 3, 4}});
	EXPECT_EQ(reinterpret_cast<std::uintptr_t>(builder.data()) % alignof(Value), 0u);
	auto buffer = std::move(builder).buildMutable();
	buffer.resize<Value>(4096);
	EXPECT_EQ(reinterpret_cast<std::uintptr_t>(buffer.data()) % alignof(Value), 0u);
	EXPECT_EQ(buffer.cast<Value>()[0].elements[3], 4u);
	buffer.cast<Value>()[4095].elements[0] = 123u;
	buffer.validate();
	const Buffer &view = buffer;
	EXPECT_EQ(view.cast<Value>()[4095].elements[0], 123u);
}
