#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>

#include "ui/widget/scroll_bar.hpp"

TEST(ScrollBarTest, OrientationRangeScaleRatioAndGeometryAreForwarded)
{
	spk::ScrollBar bar("Scroll");
	bar.setStep(0.2f);
	bar.setScale(0.4f);
	bar.setRange(-10.0f, 10.0f);
	bar.setValue(5.0f);
	bar.setGeometry({.anchor = {0, 0}, .size = {200, 20}});
	EXPECT_FLOAT_EQ(bar.step(), 0.2f);
	EXPECT_FLOAT_EQ(bar.scale(), 0.4f);
	EXPECT_FLOAT_EQ(bar.ratio(), 0.75f);
	EXPECT_FLOAT_EQ(bar.value(), 5.0f);
	EXPECT_EQ(bar.negativeButton().geometry().size, spk::Vector2UInt(20, 20));
	EXPECT_EQ(bar.slider().geometry().size, spk::Vector2UInt(160, 20));
	EXPECT_EQ(bar.positiveButton().geometry().anchor, spk::Vector2Int(180, 0));

	bar.setOrientation(spk::Orientation::Vertical);
	bar.setGeometry({.anchor = {0, 0}, .size = {20, 200}});
	EXPECT_EQ(bar.slider().orientation(), spk::Orientation::Vertical);
	EXPECT_EQ(bar.slider().geometry().size, spk::Vector2UInt(20, 160));
}

TEST(ScrollBarTest, IconConfigurationTracksOrientation)
{
	spk::ScrollBar bar("Scroll");
	const auto &style = spk::Widget::defaultStyle.get();
	ASSERT_NE(style.iconset, nullptr);
	const spk::ScrollBar::ArrowSpriteIDs ids{.up = 1, .down = 2, .left = 3, .right = 4};
	bar.setIconset(style.iconset.get());
	bar.setArrowSpriteIDs(ids);
	EXPECT_EQ(bar.iconset(), style.iconset.get());
	EXPECT_EQ(bar.arrowSpriteIDs(), ids);
	EXPECT_EQ(bar.negativeButton().iconSpriteID(), 3u);
	EXPECT_EQ(bar.positiveButton().iconSpriteID(), 4u);
	bar.setOrientation(spk::Orientation::Vertical);
	EXPECT_EQ(bar.negativeButton().iconSpriteID(), 1u);
	EXPECT_EQ(bar.positiveButton().iconSpriteID(), 2u);
}

TEST(ScrollBarTest, InvalidConfigurationThrowsWithoutChangingValues)
{
	spk::ScrollBar bar("Scroll");
	const float nan = std::numeric_limits<float>::quiet_NaN();
	EXPECT_THROW(bar.setStep(0.0f), std::invalid_argument);
	EXPECT_THROW(bar.setStep(2.0f), std::invalid_argument);
	EXPECT_THROW(bar.setStep(nan), std::invalid_argument);
	EXPECT_THROW(bar.setIconset(nullptr), std::invalid_argument);
	EXPECT_THROW(bar.setScale(0.0f), std::invalid_argument);
	EXPECT_FLOAT_EQ(bar.step(), 0.1f);
}

TEST(ScrollBarTest, InvalidStepRangeAndSpriteConfigurationPropagateExactExceptionTypes)
{
	spk::ScrollBar bar("Scroll");
	for (float value : {-1.0f, std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()})
	{
		EXPECT_THROW(bar.setStep(value), std::invalid_argument);
	}
	EXPECT_THROW(bar.setRange(10, -10), std::invalid_argument);
	const auto count = spk::Widget::defaultStyle->iconset->sprites().size();
	EXPECT_THROW(bar.setArrowSpriteIDs({.up = count, .down = count, .left = count, .right = count}), std::out_of_range);
}
