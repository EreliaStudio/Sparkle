#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <sstream>
#include <type_traits>

#include "math/vector3.hpp"

namespace
{
	constexpr double Tolerance = 1.0e-5;

	static_assert(std::is_same_v<spk::Vector3::value_type, float>);
	static_assert(std::is_same_v<spk::Vector3Int::value_type, std::int32_t>);
	static_assert(std::is_same_v<spk::Vector3UInt::value_type, std::uint32_t>);

	TEST(Vector3Test, StandardUsageConstructsConvertsAndCalculatesVectorOperations)
	{
		const spk::Vector3 first{1.0f, 2.0f, 3.0f};
		const spk::Vector3 second{4.0f, -5.0f, 6.0f};

		EXPECT_EQ(first + second, spk::Vector3(5.0f, -3.0f, 9.0f));
		EXPECT_EQ(first - second, spk::Vector3(-3.0f, 7.0f, -3.0f));
		EXPECT_EQ(first * second, spk::Vector3(4.0f, -10.0f, 18.0f));
		EXPECT_EQ(second / spk::Vector3(2.0f, 5.0f, 3.0f), spk::Vector3(2.0f, -1.0f, 2.0f));
		EXPECT_FLOAT_EQ(first.dot(second), 12.0f);
		EXPECT_EQ(first.cross(second), spk::Vector3(27.0f, 6.0f, -13.0f));
		EXPECT_NEAR(first.length(), std::sqrt(14.0), Tolerance);

		const spk::Vector3 normalized = first.normalized();
		EXPECT_NEAR(normalized.length(), 1.0, Tolerance);

		const spk::Vector3Int converted(first);
		EXPECT_EQ(converted, spk::Vector3Int(1, 2, 3));
	}

	TEST(Vector3Test, ConstructsFromVector2AndAdditionalZComponent)
	{
		const spk::Vector2Int xy{3, -4};
		const spk::Vector3 value(xy, 2.5f);

		EXPECT_EQ(value, spk::Vector3(3.0f, -4.0f, 2.5f));
	}

	TEST(Vector3Test, CompoundAssignmentsModifyEveryComponent)
	{
		spk::Vector3Int value{8, -12, 18};

		value += {2, 2, 2};
		EXPECT_EQ(value, spk::Vector3Int(10, -10, 20));
		value -= {5, 0, 5};
		EXPECT_EQ(value, spk::Vector3Int(5, -10, 15));
		value *= {2, -1, 2};
		EXPECT_EQ(value, spk::Vector3Int(10, 10, 30));
		value /= {2, 5, 3};
		EXPECT_EQ(value, spk::Vector3Int(5, 2, 10));
	}

	TEST(Vector3Test, CrossProductUsesRightHandedOrientation)
	{
		const spk::Vector3 xAxis{1.0f, 0.0f, 0.0f};
		const spk::Vector3 yAxis{0.0f, 1.0f, 0.0f};
		const spk::Vector3 zAxis{0.0f, 0.0f, 1.0f};

		EXPECT_EQ(xAxis.cross(yAxis), zAxis);
		EXPECT_EQ(yAxis.cross(xAxis), -zAxis);
	}

	TEST(Vector3Test, DotAndCrossCoverParallelAntiparallelAndOrthogonalVectors)
	{
		const spk::Vector3 axis{2.0f, 0.0f, 0.0f};
		const spk::Vector3 parallel{4.0f, 0.0f, 0.0f};
		const spk::Vector3 antiparallel{-4.0f, 0.0f, 0.0f};
		const spk::Vector3 orthogonal{0.0f, 3.0f, 0.0f};

		EXPECT_FLOAT_EQ(axis.dot(parallel), 8.0f);
		EXPECT_FLOAT_EQ(axis.dot(antiparallel), -8.0f);
		EXPECT_FLOAT_EQ(axis.dot(orthogonal), 0.0f);
		EXPECT_EQ(axis.cross(parallel), spk::Vector3{});
		EXPECT_EQ(axis.cross(antiparallel), spk::Vector3{});
		EXPECT_EQ(axis.cross(orthogonal), spk::Vector3(0.0f, 0.0f, 6.0f));
	}

	TEST(Vector3Test, NormalizesNegativeSmallAndLargeFiniteVectors)
	{
		for (const spk::Vector3 value : {
				 spk::Vector3{-2.0f, -3.0f, -6.0f},
				 spk::Vector3{1.0e-18f, -2.0e-18f, 3.0e-18f},
				 spk::Vector3{1.0e18f, -2.0e18f, 3.0e18f}})
		{
			const spk::Vector3 normalized = value.normalized();
			EXPECT_NEAR(normalized.length(), 1.0, 1.0e-4);
		}
	}

	TEST(Vector3Test, NormalizeZeroLengthFloatingVectorThrowsDomainError)
	{
		EXPECT_THROW((void)spk::Vector3{}.normalized(), std::domain_error);
	}

	TEST(Vector3Test, DataArrayAliasesNamedComponents)
	{
		spk::Vector3Int value{1, 2, 3};

		EXPECT_EQ(value.data[0], 1);
		EXPECT_EQ(value.data[1], 2);
		EXPECT_EQ(value.data[2], 3);

		value.data[1] = -9;
		EXPECT_EQ(value.y, -9);
	}

	TEST(Vector3Test, EqualityIncludesEveryComponent)
	{
		EXPECT_EQ(spk::Vector3UInt(1u, 2u, 3u), spk::Vector3UInt(1u, 2u, 3u));
		EXPECT_NE(spk::Vector3UInt(1u, 2u, 3u), spk::Vector3UInt(1u, 3u, 2u));
	}

	TEST(Vector3Test, StreamsAllAliasesConsistently)
	{
		std::ostringstream floatingStream;
		std::ostringstream signedStream;
		std::ostringstream unsignedStream;

		floatingStream << spk::Vector3{1.0f, 2.0f, 3.0f};
		signedStream << spk::Vector3Int{-1, 2, -3};
		unsignedStream << spk::Vector3UInt{1u, 2u, 3u};

		EXPECT_EQ(floatingStream.str(), "(1, 2, 3)");
		EXPECT_EQ(signedStream.str(), "(-1, 2, -3)");
		EXPECT_EQ(unsignedStream.str(), "(1, 2, 3)");
	}
}
