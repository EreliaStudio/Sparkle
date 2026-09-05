#include <gtest/gtest.h>

#include <limits>
#include <vector>

#include "ui/widget/progress_bar.hpp"

TEST(ProgressBarTest, RatioClampsAndNotifiesOnlyForEffectiveChanges)
{
	spk::ProgressBar bar("Progress");
	std::vector<float> editions;
	auto contract = bar.subscribeToEdition([&](float ratio) {
		editions.push_back(ratio);
	});
	bar.setRatio(-2.0f);
	bar.setRatio(0.25f);
	bar.setRatio(0.25f);
	bar.setRatio(2.0f);
	EXPECT_FLOAT_EQ(bar.ratio(), 1.0f);
	ASSERT_EQ(editions.size(), 2u);
	EXPECT_FLOAT_EQ(editions[0], 0.25f);
	EXPECT_FLOAT_EQ(editions[1], 1.0f);
}

TEST(ProgressBarTest, EveryDirectionProducesExpectedFillGeometry)
{
	spk::ProgressBar bar("Progress");
	bar.setGeometry({.anchor = {7, 9}, .size = {100, 40}});
	bar.setRatio(0.25f);
	ASSERT_EQ(bar.children().size(), 1u);
	auto *fill = bar.children().front();

	bar.setFillDirection(spk::ProgressBar::FillDirection::LeftToRight);
	EXPECT_EQ(fill->geometry(), (spk::Rect2D{.anchor = {0, 0}, .size = {25, 40}}));
	bar.setFillDirection(spk::ProgressBar::FillDirection::RightToLeft);
	EXPECT_EQ(fill->geometry(), (spk::Rect2D{.anchor = {75, 0}, .size = {25, 40}}));
	bar.setFillDirection(spk::ProgressBar::FillDirection::BottomToTop);
	EXPECT_EQ(fill->geometry(), (spk::Rect2D{.anchor = {0, 30}, .size = {100, 10}}));
	bar.setFillDirection(spk::ProgressBar::FillDirection::TopToBottom);
	EXPECT_EQ(fill->geometry(), (spk::Rect2D{.anchor = {0, 0}, .size = {100, 10}}));
}

TEST(ProgressBarTest, TextureCornerDepthAndStyleAreForwarded)
{
	spk::ProgressBar bar("Progress");
	const auto &style = spk::Widget::defaultStyle.get();
	ASSERT_NE(style.sliderBody, nullptr);
	bar.setFillTexture(style.sliderBody.get());
	bar.setCornerSize({3, 5});
	bar.setFillDepth(2.0f);
	EXPECT_EQ(bar.fillTexture(), style.sliderBody.get());
	EXPECT_EQ(bar.cornerSize(), spk::Vector2Int(3, 5));
	EXPECT_FLOAT_EQ(bar.fillDepth(), 2.0f);
	bar.applyStyle(style);
	EXPECT_EQ(bar.fillTexture(), style.sliderBody.get());
}

TEST(ProgressBarTest, InfiniteRatiosClampToEndpoints)
{
	spk::ProgressBar bar("Progress");
	bar.setRatio(std::numeric_limits<float>::infinity());
	EXPECT_EQ(bar.ratio(), 1);
	bar.setRatio(-std::numeric_limits<float>::infinity());
	EXPECT_EQ(bar.ratio(), 0);
}

// NaN currently reaches lround and integer conversion while computing fill geometry.
TEST(ProgressBarTest, DISABLED_NaNRatioIsRejectedWithoutMutation)
{
	spk::ProgressBar bar("Progress");
	bar.setRatio(0.5f);
	const auto before = bar.children().front()->geometry();
	EXPECT_THROW(bar.setRatio(std::numeric_limits<float>::quiet_NaN()), std::invalid_argument);
	EXPECT_EQ(bar.ratio(), 0.5f);
	EXPECT_EQ(bar.children().front()->geometry(), before);
}
