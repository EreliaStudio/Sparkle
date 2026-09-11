#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <sstream>
#include <type_traits>

#include "math/vector4.hpp"

namespace
{
	static_assert(std::is_trivially_copyable_v<spk::Vector4>);
	static_assert(std::is_standard_layout_v<spk::Vector4>);
	static_assert(sizeof(spk::Vector4) == sizeof(float) * 4);
	static_assert(sizeof(spk::Vector4Int) == sizeof(std::int32_t) * 4);
	static_assert(sizeof(spk::Vector4UInt) == sizeof(std::uint32_t) * 4);

	TEST(Vector4Test, StandardUsageConstructsConvertsAndPerformsComponentWiseArithmetic)
	{
		spk::Vector4 value{1.0f, 2.0f, 3.0f, 1.0f};
		const spk::Vector4 operand{2.0f, 4.0f, 6.0f, 2.0f};

		EXPECT_EQ(value + operand, spk::Vector4(3.0f, 6.0f, 9.0f, 3.0f));
		EXPECT_EQ(value - operand, spk::Vector4(-1.0f, -2.0f, -3.0f, -1.0f));
		EXPECT_EQ(value * operand, spk::Vector4(2.0f, 8.0f, 18.0f, 2.0f));
		EXPECT_EQ(operand / spk::Vector4(2.0f, 2.0f, 3.0f, 2.0f), spk::Vector4(1.0f, 2.0f, 2.0f, 1.0f));

		value += operand;
		EXPECT_EQ(value, spk::Vector4(3.0f, 6.0f, 9.0f, 3.0f));
		value -= operand;
		EXPECT_EQ(value, spk::Vector4(1.0f, 2.0f, 3.0f, 1.0f));
		value *= operand;
		EXPECT_EQ(value, spk::Vector4(2.0f, 8.0f, 18.0f, 2.0f));
		value /= operand;
		EXPECT_EQ(value, spk::Vector4(1.0f, 2.0f, 3.0f, 1.0f));

		EXPECT_EQ(spk::Vector4Int(value), spk::Vector4Int(1, 2, 3, 1));
	}

	TEST(Vector4Test, ConstructsFromVector2AndVector3)
	{
		const spk::Vector2Int xy{2, -3};
		const spk::Vector3Int xyz{2, -3, 4};

		EXPECT_EQ(spk::Vector4(xy, 4.0f, 1.0f), spk::Vector4(2.0f, -3.0f, 4.0f, 1.0f));
		EXPECT_EQ(spk::Vector4(xyz, 0.0f), spk::Vector4(2.0f, -3.0f, 4.0f, 0.0f));
	}

	TEST(Vector4Test, HomogeneousWValuesArePreserved)
	{
		const spk::Vector4 point{1.0f, 2.0f, 3.0f, 1.0f};
		const spk::Vector4 direction{1.0f, 2.0f, 3.0f, 0.0f};
		const spk::Vector4 custom{1.0f, 2.0f, 3.0f, -2.0f};

		EXPECT_FLOAT_EQ(point.w, 1.0f);
		EXPECT_FLOAT_EQ(direction.w, 0.0f);
		EXPECT_FLOAT_EQ(custom.w, -2.0f);
	}

	TEST(Vector4Test, DefaultConstructionProducesAllZeroComponents)
	{
		EXPECT_EQ(spk::Vector4{}, spk::Vector4(0.0f, 0.0f, 0.0f, 0.0f));
	}

	TEST(Vector4Test, DataArrayAliasesNamedComponents)
	{
		spk::Vector4Int value{1, 2, 3, 4};

		EXPECT_EQ(value.data[0], value.x);
		EXPECT_EQ(value.data[1], value.y);
		EXPECT_EQ(value.data[2], value.z);
		EXPECT_EQ(value.data[3], value.w);

		value.data[3] = -5;
		EXPECT_EQ(value.w, -5);
	}

	TEST(Vector4Test, EqualityIncludesAllFourComponents)
	{
		EXPECT_EQ(spk::Vector4Int(1, 2, 3, 4), spk::Vector4Int(1, 2, 3, 4));
		EXPECT_NE(spk::Vector4Int(1, 2, 3, 4), spk::Vector4Int(1, 2, 3, 5));
	}

	TEST(Vector4Test, NegativeAndExtremeValuesCanBeRepresentedWithoutOverflowingArithmetic)
	{
		const spk::Vector4Int signedValue{
			std::numeric_limits<std::int32_t>::lowest(),
			-1,
			0,
			std::numeric_limits<std::int32_t>::max()};
		const spk::Vector4UInt unsignedValue{
			0u,
			1u,
			std::numeric_limits<std::uint32_t>::max() - 1u,
			std::numeric_limits<std::uint32_t>::max()};

		EXPECT_EQ(signedValue.x, std::numeric_limits<std::int32_t>::lowest());
		EXPECT_EQ(signedValue.w, std::numeric_limits<std::int32_t>::max());
		EXPECT_EQ(unsignedValue.z, std::numeric_limits<std::uint32_t>::max() - 1u);
		EXPECT_EQ(unsignedValue.w, std::numeric_limits<std::uint32_t>::max());
	}

	TEST(Vector4Test, StreamsUsingParenthesizedCommaSeparatedComponents)
	{
		std::ostringstream stream;
		stream << spk::Vector4Int{-1, 2, -3, 4};

		EXPECT_EQ(stream.str(), "(-1, 2, -3, 4)");
	}

	TEST(Vector4Test, ToJSONSerializesComponentsAsArray)
	{
		const spk::Vector4 value{1.5f, -2.25f, -3.125f, 4.25};

		const spk::JSON::Value json = value.toJSON();
		const auto &array = json.asArray();

		ASSERT_EQ(array.size(), 4u);
		EXPECT_FLOAT_EQ(array[0].as<float>(), 1.5f);
		EXPECT_FLOAT_EQ(array[1].as<float>(), -2.25f);
		EXPECT_FLOAT_EQ(array[2].as<float>(), -3.125f);
		EXPECT_FLOAT_EQ(array[3].as<float>(), 4.25f);
	}

	TEST(Vector4Test, FromJSONDeserializesComponentsFromArray)
	{
		spk::JSON::Value json = spk::JSON::Value::array();
		json.pushBack(1.5f);
		json.pushBack(-2.25f);
		json.pushBack(-3.125f);
		json.pushBack(4.25f);

		const spk::Vector4 value = spk::Vector4::fromJSON(json);

		EXPECT_EQ(value, spk::Vector4(1.5f, -2.25f, -3.125f, 4.25f));
	}

	TEST(Vector4Test, ConstructorFromJSONDeserializesComponentsFromArray)
	{
		spk::JSON::Value json = spk::JSON::Value::array();
		json.pushBack(3.5f);
		json.pushBack(-4.25f);
		json.pushBack(-5.125f);
		json.pushBack(4.25f);

		const spk::Vector4 value = spk::Vector4(json);

		EXPECT_EQ(value, spk::Vector4(3.5f, -4.25f, -5.125f, 4.25f));
	}

	TEST(Vector4Test, JSONRoundTripPreservesValue)
	{
		const spk::Vector4 original{3.25f, -7.5f, -5.125f, 4.25f};

		const spk::Vector4 result =
			spk::Vector4::fromJSON(original.toJSON());

		EXPECT_EQ(result, original);
	}
}
