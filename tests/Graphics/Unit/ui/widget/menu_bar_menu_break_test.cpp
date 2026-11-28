#include <gtest/gtest.h>

#include <filesystem>
#include <string>

#include "rendering/render_snapshot.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"
#include "ui/widget/menu_bar.hpp"

namespace
{
	const spk::Widget::Style &defaultStyle()
	{
		return spk::Widget::defaultStyle.get();
	}

	void expectWidgetImage(spk::Widget &widget, const std::filesystem::path &category, const std::string &name)
	{
		auto &context = sparkle_test::OpenGLTestContext::instance();
		context.reset();
		spk::RenderSnapshot::Builder builder;
		widget.buildRenderSnapshot(builder);
		const auto snapshot = builder.build();
		snapshot.execute(context.renderContext());

		const auto actual = sparkle_test::resultImagePath(category, name);
		const auto expected = sparkle_test::expectedImagePath(category, name);
		const auto difference = sparkle_test::resultImagePath(category, name + "_difference");
		context.save(actual);
		ASSERT_TRUE(std::filesystem::exists(expected))
			<< "Missing golden image: " << expected << "\n"
			<< "The current render was saved to: " << actual;
		const auto result = sparkle_test::compareImages(actual, expected, difference);
		EXPECT_TRUE(result.matches)
			<< "Different pixels: " << result.differentPixelCount << "\n"
			<< "Actual image: " << actual << "\n"
			<< "Expected image: " << expected << "\n"
			<< "Difference image: " << difference;
	}
}

TEST(MenuBarBreakTest, DefaultConfiguredAndStyleState)
{
	spk::MenuBar::Menu::Break separator("Break");
	EXPECT_EQ(separator.spriteSheet(), defaultStyle().menuBreak.get());
	EXPECT_EQ(separator.height(), 2u);
	EXPECT_FLOAT_EQ(separator.depth(), 0.0f);
	separator.setHeight(5);
	separator.setDepth(2.0f);
	separator.applyStyle(defaultStyle());
	EXPECT_EQ(separator.spriteSheet(), defaultStyle().menuBreak.get());
	EXPECT_EQ(separator.height(), 5u);
	EXPECT_FLOAT_EQ(separator.depth(), 2.0f);
}

TEST(MenuBarBreakTest, InvalidSpriteSheetsAreRejected)
{
	spk::MenuBar::Menu::Break separator("Break");
	EXPECT_THROW(separator.setSpriteSheet(nullptr), std::invalid_argument);
	EXPECT_THROW(separator.setSpriteSheet(defaultStyle().iconset.get()), std::invalid_argument);
}

TEST(MenuBarBreakRenderTest, ThreePartSeparator)
{
	spk::MenuBar::Menu::Break separator("Break");
	separator.setHeight(8);
	separator.setDepth(0.25f);
	separator.setGeometry({.anchor = {30, 30}, .size = {300, 8}});
	expectWidgetImage(separator, "ui/widget/menu_bar_break", "three_part_separator");
}
