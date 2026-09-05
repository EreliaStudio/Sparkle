#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <sstream>
#include <type_traits>

#include "math/vector2.hpp"

namespace
{
	static_assert(std::is_same_v<spk::Vector2::value_type, float_t>);
	static_assert(std::is_same_v<spk::Vector2Int::value_type, std::int32_t>);
	static_assert(std::is_same_v<spk::Vector2UInt::value_type, std::uint32_t>);

	TEST(Vector2Test, StandardUsageConstructsConvertsAndPerformsComponentWiseArithmetic)
	{
		spk::Vector2 first{1.5f, -2.0f};
		const spk::Vector2 second{2.0f, 4.0f};

		EXPECT_EQ(first + second, spk::Vector2(3.5f, 2.0f));
		EXPECT_EQ(first - second, spk::Vector2(-0.5f, -6.0f));
		EXPECT_EQ(first * second, spk::Vector2(3.0f, -8.0f));
		EXPECT_EQ(first / second, spk::Vector2(0.75f, -0.5f));

		first += second;
		EXPECT_EQ(first, spk::Vector2(3.5f, 2.0f));
		first -= second;
		EXPECT_EQ(first, spk::Vector2(1.5f, -2.0f));
		first *= second;
		EXPECT_EQ(first, spk::Vector2(3.0f, -8.0f));
		first /= second;
		EXPECT_EQ(first, spk::Vector2(1.5f, -2.0f));

		const spk::Vector2Int converted(first);
		EXPECT_EQ(converted, spk::Vector2Int(1, -2));
	}

	TEST(Vector2Test, SignedAndUnsignedAliasesUseNativeComponentArithmetic)
	{
		spk::Vector2Int signedValue{12, -18};
		const spk::Vector2Int signedOperand{3, 6};
		EXPECT_EQ(signedValue / signedOperand, spk::Vector2Int(4, -3));
		signedValue += spk::Vector2Int{-2, 8};
		EXPECT_EQ(signedValue, spk::Vector2Int(10, -10));

		spk::Vector2UInt unsignedValue{12u, 18u};
		const spk::Vector2UInt unsignedOperand{3u, 6u};
		EXPECT_EQ(unsignedValue / unsignedOperand, spk::Vector2UInt(4u, 3u));
		unsignedValue *= spk::Vector2UInt{2u, 3u};
		EXPECT_EQ(unsignedValue, spk::Vector2UInt(24u, 54u));
	}

	TEST(Vector2Test, DefaultConstructionProducesZeroVector)
	{
		EXPECT_EQ(spk::Vector2{}, spk::Vector2(0.0f, 0.0f));
		EXPECT_EQ(spk::Vector2Int{}, spk::Vector2Int(0, 0));
		EXPECT_EQ(spk::Vector2UInt{}, spk::Vector2UInt(0u, 0u));
	}

	TEST(Vector2Test, ConversionUsesNativeStaticCastSemantics)
	{
		const spk::Vector2 source{3.75f, -8.25f};
		const spk::Vector2Int integers(source);
		const spk::Vector2 roundTrip(integers);

		EXPECT_EQ(integers, spk::Vector2Int(3, -8));
		EXPECT_EQ(roundTrip, spk::Vector2(3.0f, -8.0f));
	}

	TEST(Vector2Test, DataArrayAliasesNamedComponents)
	{
		spk::Vector2Int value{4, 9};

		EXPECT_EQ(value.data[0], value.x);
		EXPECT_EQ(value.data[1], value.y);

		value.data[0] = -7;
		value.data[1] = 12;

		EXPECT_EQ(value.x, -7);
		EXPECT_EQ(value.y, 12);
	}

	TEST(Vector2Test, EqualityComparesBothComponents)
	{
		EXPECT_EQ(spk::Vector2Int(1, 2), spk::Vector2Int(1, 2));
		EXPECT_NE(spk::Vector2Int(1, 2), spk::Vector2Int(2, 1));
	}

	TEST(Vector2Test, FractionalAndNegativeFloatingPointValuesArePreserved)
	{
		const spk::Vector2 value{-0.25f, 0.125f};

		EXPECT_FLOAT_EQ(value.x, -0.25f);
		EXPECT_FLOAT_EQ(value.y, 0.125f);
	}

	TEST(Vector2Test, ExtremaCanBeStoredWithoutPerformingOverflowingArithmetic)
	{
		const spk::Vector2Int signedExtrema{
			std::numeric_limits<std::int32_t>::lowest(),
			std::numeric_limits<std::int32_t>::max()};
		const spk::Vector2UInt unsignedExtrema{
			0u,
			std::numeric_limits<std::uint32_t>::max()};

		EXPECT_EQ(signedExtrema.x, std::numeric_limits<std::int32_t>::lowest());
		EXPECT_EQ(signedExtrema.y, std::numeric_limits<std::int32_t>::max());
		EXPECT_EQ(unsignedExtrema.x, 0u);
		EXPECT_EQ(unsignedExtrema.y, std::numeric_limits<std::uint32_t>::max());
	}

	TEST(Vector2Test, StreamsUsingParenthesizedCommaSeparatedComponents)
	{
		std::ostringstream stream;
		stream << spk::Vector2Int{-3, 7};

		EXPECT_EQ(stream.str(), "(-3, 7)");
	}
}
