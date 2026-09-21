#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "ui/widget/progress_bar.hpp"

TEST(LabeledProgressBarTest, DefaultAndCustomPredicatesRefreshAfterRatioChanges)
{
	spk::LabeledProgressBar bar("Progress");
	EXPECT_EQ(bar.ratio(), 0.0f);
	EXPECT_EQ(bar.label().text(), U"0%");

	bar.setRatio(0.426f);
	EXPECT_FLOAT_EQ(bar.ratio(), 0.426f);
	EXPECT_EQ(bar.label().text(), U"43%");

	bar.setTextPredicate([](float ratio) {
		return std::string("ratio=") + std::to_string(ratio).substr(0, 4);
	});
	EXPECT_EQ(bar.label().text(), U"ratio=0.42");
	bar.setRatio(1.0f);
	EXPECT_EQ(bar.label().text(), U"ratio=1.00");
}

TEST(LabeledProgressBarTest, PredicateCanBeClearedReplacedAndRecoveredAfterFailure)
{
	spk::LabeledProgressBar bar("Progress");
	bar.setRatio(0.5f);
	bar.setTextPredicate({});
	EXPECT_TRUE(bar.label().text().empty());

	bar.setTextPredicate([](float) { return "stable"; });
	EXPECT_EQ(bar.label().text(), U"stable");
	EXPECT_THROW(
		bar.setTextPredicate([](float) -> std::string { throw std::runtime_error("predicate failure"); }),
		std::runtime_error);
	EXPECT_FLOAT_EQ(bar.ratio(), 0.5f);
	EXPECT_EQ(bar.label().text(), U"stable");

	bar.setTextPredicate([](float ratio) { return ratio == 0.5f ? "recovered" : "changed"; });
	EXPECT_EQ(bar.label().text(), U"recovered");
}

TEST(LabeledProgressBarTest, PropertiesGeometryAndEditionAreForwarded)
{
	spk::LabeledProgressBar bar("Progress");
	int editions = 0;
	float observed = -1.0f;
	{
		auto contract = bar.subscribeToEdition([&](float ratio) {
			++editions;
			observed = ratio;
		});
		bar.setRatio(0.25f);
		bar.setRatio(0.25f);
		EXPECT_EQ(editions, 1);
		EXPECT_FLOAT_EQ(observed, 0.25f);
	}
	bar.setRatio(0.75f);
	EXPECT_EQ(editions, 1);

	bar.setFillDirection(spk::ProgressBar::FillDirection::TopToBottom);
	bar.setCornerSize({4, 6});
	bar.setFillDepth(3.5f);
	EXPECT_EQ(bar.fillDirection(), spk::ProgressBar::FillDirection::TopToBottom);
	EXPECT_EQ(bar.cornerSize(), spk::Vector2Int(4, 6));
	EXPECT_FLOAT_EQ(bar.fillDepth(), 3.5f);

	bar.setGeometry({.anchor = {13, 17}, .size = {180, 40}});
	EXPECT_EQ(bar.label().geometry(), (spk::Rect2D{.anchor = {0, 0}, .size = {180, 40}}));
	ASSERT_EQ(bar.children().size(), 2u);
	for (const spk::Widget *child : bar.children())
	{
		EXPECT_EQ(child->geometry(), (spk::Rect2D{.anchor = {0, 0}, .size = {180, 40}}));
	}
	EXPECT_GT(bar.label().absoluteZOrder(), bar.children().front()->absoluteZOrder());
}
