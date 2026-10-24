#include <gtest/gtest.h>

#include <stdexcept>

#include "ui/widget/text_label.hpp"

TEST(TextLabelTest, TextPresentationAccessorsAndMeasuredHintStaySynchronized)
{
	spk::TextLabel label("Label");
	const auto *font = spk::Widget::defaultStyle.get().font.get();
	ASSERT_NE(font, nullptr);
	label.setText(U"Hello ✓");
	label.setTextSize({18, 2});
	label.setGlyphColor({0.2f, 0.3f, 0.4f, 1.0f});
	label.setOutlineColor({0.8f, 0.7f, 0.6f, 1.0f});
	label.setAlignment({spk::Alignment::Horizontal::Right, spk::Alignment::Vertical::Bottom});
	label.setPadding({5, 7});
	label.setDepth(4.5f);
	EXPECT_EQ(label.font(), font);
	EXPECT_EQ(label.text(), U"Hello ✓");
	EXPECT_EQ(label.textSize(), (spk::Font::Size{18, 2}));
	EXPECT_EQ(label.glyphColor(), (spk::Color{0.2f, 0.3f, 0.4f, 1.0f}));
	EXPECT_EQ(label.outlineColor(), (spk::Color{0.8f, 0.7f, 0.6f, 1.0f}));
	EXPECT_EQ(label.alignment(), (spk::Alignment{spk::Alignment::Horizontal::Right, spk::Alignment::Vertical::Bottom}));
	EXPECT_EQ(label.padding(), spk::Vector2UInt(5, 7));
	EXPECT_FLOAT_EQ(label.depth(), 4.5f);
	EXPECT_GT(label.preferredSize().x, 10.0f);
	EXPECT_GT(label.preferredSize().y, 14.0f);
}

TEST(TextLabelTest, EmptyUnicodeAndAlignmentBoundariesAreSupported)
{
	spk::TextLabel label("Label");
	label.setText("");
	EXPECT_EQ(label.preferredSize(), spk::Vector2(0, 0));
	label.setText("é漢字");
	EXPECT_FALSE(label.text().empty());
	for (auto horizontal : {spk::Alignment::Horizontal::Left, spk::Alignment::Horizontal::Center, spk::Alignment::Horizontal::Right})
	{
		label.setHorizontalAlignment(horizontal);
		EXPECT_EQ(label.horizontalAlignment(), horizontal);
	}
	for (auto vertical : {spk::Alignment::Vertical::Top, spk::Alignment::Vertical::Center, spk::Alignment::Vertical::Bottom})
	{
		label.setVerticalAlignment(vertical);
		EXPECT_EQ(label.verticalAlignment(), vertical);
	}
}

TEST(TextLabelTest, NullFontThrowsWithoutReplacingTheCurrentFont)
{
	spk::TextLabel label("Label");
	auto *font = label.font();
	EXPECT_THROW(label.setFont(nullptr), std::invalid_argument);
	EXPECT_EQ(label.font(), font);
}
