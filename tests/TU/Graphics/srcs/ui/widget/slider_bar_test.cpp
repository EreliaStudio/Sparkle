#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>

#include "ui/widget/slider_bar.hpp"

TEST(SliderBarTest, RangeValueRatioAndClampingStaySynchronized)
{
	spk::SliderBar slider("Slider");
	slider.setRange(-20.0f, 80.0f);
	slider.setValue(30.0f);
	EXPECT_FLOAT_EQ(slider.ratio(), 0.5f);
	EXPECT_FLOAT_EQ(slider.value(), 30.0f);
	slider.setValue(-100.0f);
	EXPECT_FLOAT_EQ(slider.value(), -20.0f);
	slider.setRatio(2.0f);
	EXPECT_FLOAT_EQ(slider.value(), 80.0f);
	slider.setRange(4.0f, 4.0f);
	slider.setValue(100.0f);
	EXPECT_FLOAT_EQ(slider.ratio(), 0.0f);
	EXPECT_FLOAT_EQ(slider.value(), 4.0f);
}

TEST(SliderBarTest, GeometryScaleAndOrientationPositionTheBody)
{
	spk::SliderBar slider("Slider");
	slider.setScale(0.25f);
	slider.setGeometry({.anchor = {0, 0}, .size = {200, 20}});
	slider.setRatio(0.5f);
	EXPECT_EQ(slider.background().geometry().size, spk::Vector2UInt(200, 20));
	EXPECT_EQ(slider.body().geometry(), (spk::Rect2D{.anchor = {75, 0}, .size = {50, 20}}));
	slider.setOrientation(spk::Orientation::Vertical);
	slider.setGeometry({.anchor = {0, 0}, .size = {20, 200}});
	EXPECT_EQ(slider.body().geometry(), (spk::Rect2D{.anchor = {0, 75}, .size = {20, 50}}));
}

TEST(SliderBarTest, InvalidNumericConfigurationIsAtomic)
{
	spk::SliderBar slider("Slider");
	const float nan = std::numeric_limits<float>::quiet_NaN();
	const float infinity = std::numeric_limits<float>::infinity();
	EXPECT_THROW(slider.setScale(0.0f), std::invalid_argument);
	EXPECT_THROW(slider.setScale(1.1f), std::invalid_argument);
	EXPECT_THROW(slider.setScale(nan), std::invalid_argument);
	EXPECT_THROW(slider.setRange(2.0f, 1.0f), std::invalid_argument);
	EXPECT_THROW(slider.setRange(-infinity, 1.0f), std::invalid_argument);
	EXPECT_THROW(slider.setRatio(nan), std::invalid_argument);
	EXPECT_THROW(slider.setValue(infinity), std::invalid_argument);
	EXPECT_FLOAT_EQ(slider.scale(), 0.1f);
	EXPECT_EQ(slider.range(), (spk::SliderBar::Range{0.0f, 100.0f}));
	EXPECT_FLOAT_EQ(slider.ratio(), 0.0f);
}
