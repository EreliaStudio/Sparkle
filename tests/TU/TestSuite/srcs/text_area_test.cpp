#include <gtest/gtest.h>

#include <stdexcept>

#include "ui/widget/text_area.hpp"

TEST(TextAreaTest, WrappedMultilineSizingRespondsToWidthAndLinePadding)
{
	spk::TextArea area("Area");
	area.setText("alpha beta gamma\nsecond line");
	area.setTextSize({16, 1});
	const auto wide = area.computePreferredSize(400);
	const auto narrow = area.computePreferredSize(60);
	EXPECT_GT(narrow.y, wide.y);
	area.setLinePadding(6);
	EXPECT_GT(area.computePreferredSize(60).y, narrow.y);
	area.setMinimalWidth(80);
	EXPECT_GE(area.computePreferredSize(40).x, 80u);
}

TEST(TextAreaTest, PresentationUnicodeEmptyAndAlignmentCasesRoundTrip)
{
	spk::TextArea area("Area");
	area.setText(U"é漢字");
	area.setGlyphColor({0.1f, 0.2f, 0.3f, 1.0f});
	area.setOutlineColor({0.9f, 0.8f, 0.7f, 1.0f});
	area.setDepth(-2.0f);
	area.setAlignment({spk::Alignment::Horizontal::Center, spk::Alignment::Vertical::Bottom});
	EXPECT_EQ(area.text(), U"é漢字");
	EXPECT_EQ(area.glyphColor(), (spk::Color{0.1f, 0.2f, 0.3f, 1.0f}));
	EXPECT_EQ(area.outlineColor(), (spk::Color{0.9f, 0.8f, 0.7f, 1.0f}));
	EXPECT_FLOAT_EQ(area.depth(), -2.0f);
	EXPECT_EQ(area.alignment(), (spk::Alignment{spk::Alignment::Horizontal::Center, spk::Alignment::Vertical::Bottom}));
	area.setText("");
	EXPECT_EQ(area.computePreferredSize(100).y, 0u);
}

TEST(TextAreaTest, NullFontThrowsWithoutPartialMutation)
{
	spk::TextArea area("Area");
	auto *font = area.font();
	area.setText("stable");
	EXPECT_THROW(area.setFont(nullptr), std::invalid_argument);
	EXPECT_EQ(area.font(), font);
	EXPECT_EQ(area.text(), U"stable");
}
