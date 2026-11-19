#include <gtest/gtest.h>

#include <type_traits>

#include "graphics/color.hpp"

namespace
{
	static_assert(std::is_same_v<spk::Color, spk::RGBA>);
	static_assert(!std::is_same_v<spk::RGB, spk::RGBA>);
	static_assert(std::is_aggregate_v<spk::RGB>);
	static_assert(std::is_aggregate_v<spk::RGBA>);
	static_assert(std::is_trivially_copyable_v<spk::RGB>);
	static_assert(std::is_trivially_copyable_v<spk::RGBA>);
	static_assert(sizeof(spk::RGB) == 3 * sizeof(float));
	static_assert(sizeof(spk::RGBA) == 4 * sizeof(float));
}

TEST(GraphicsColorTest, RgbDefaultChannelsAreZero)
{
	const spk::RGB value{};
	EXPECT_FLOAT_EQ(value.r, 0.0f);
	EXPECT_FLOAT_EQ(value.g, 0.0f);
	EXPECT_FLOAT_EQ(value.b, 0.0f);
}

TEST(GraphicsColorTest, RgbaDefaultAlphaIsOpaque)
{
	const spk::RGBA value{};
	EXPECT_FLOAT_EQ(value.r, 0.0f);
	EXPECT_FLOAT_EQ(value.g, 0.0f);
	EXPECT_FLOAT_EQ(value.b, 0.0f);
	EXPECT_FLOAT_EQ(value.a, 1.0f);
}

TEST(GraphicsColorTest, OmittedAlphaPreservesOpaqueDefault)
{
	const spk::Color value{0.2f, 0.4f, 0.6f};
	EXPECT_FLOAT_EQ(value.a, 1.0f);
}

TEST(GraphicsColorTest, RgbAggregateRetainsChannelOrder)
{
	const spk::RGB value{0.25f, 0.5f, 0.75f};
	EXPECT_FLOAT_EQ(value.r, 0.25f);
	EXPECT_FLOAT_EQ(value.g, 0.5f);
	EXPECT_FLOAT_EQ(value.b, 0.75f);
}

TEST(GraphicsColorTest, RgbaAggregateRetainsChannelOrderAndExplicitAlpha)
{
	const spk::RGBA value{0.25f, 0.5f, 0.75f, 0.125f};
	EXPECT_FLOAT_EQ(value.r, 0.25f);
	EXPECT_FLOAT_EQ(value.g, 0.5f);
	EXPECT_FLOAT_EQ(value.b, 0.75f);
	EXPECT_FLOAT_EQ(value.a, 0.125f);
}

TEST(GraphicsColorTest, RgbEqualityComparesAllThreeChannels)
{
	const spk::RGB baseline{0.1f, 0.2f, 0.3f};
	EXPECT_EQ(baseline, (spk::RGB{0.1f, 0.2f, 0.3f}));
	EXPECT_NE(baseline, (spk::RGB{0.4f, 0.2f, 0.3f}));
	EXPECT_NE(baseline, (spk::RGB{0.1f, 0.4f, 0.3f}));
	EXPECT_NE(baseline, (spk::RGB{0.1f, 0.2f, 0.4f}));
}

TEST(GraphicsColorTest, RgbaEqualityAlsoComparesAlpha)
{
	const spk::Color baseline{0.1f, 0.2f, 0.3f, 0.4f};
	EXPECT_EQ(baseline, (spk::RGBA{0.1f, 0.2f, 0.3f, 0.4f}));
	EXPECT_NE(baseline, (spk::RGBA{0.1f, 0.2f, 0.3f, 0.5f}));
	EXPECT_NE(baseline, (spk::RGBA{0.1f, 0.2f, 0.5f, 0.4f}));
}

TEST(GraphicsColorTest, ColorAliasMaintainsRgbaCopySemantics)
{
	const spk::RGBA original{0.1f, 0.2f, 0.3f, 0.4f};
	spk::Color copy = original;
	copy.a = 0.9f;
	EXPECT_EQ(original.a, 0.4f);
	EXPECT_EQ(copy.a, 0.9f);
}
