#include "rendering/render_snapshot.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"
#include "ui/widget/menu_bar.hpp"
#include <filesystem>
#include <gtest/gtest.h>

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
		builder.build().execute(context.renderContext());
		const auto actual = sparkle_test::resultImagePath(category, name);
		const auto expected = sparkle_test::expectedImagePath(category, name);
		const auto difference = sparkle_test::resultImagePath(category, name + "_difference");
		context.save(actual);
		ASSERT_TRUE(std::filesystem::exists(expected)) << "Missing golden image: " << expected;
		EXPECT_TRUE(sparkle_test::compareImages(actual, expected, difference).matches);
	}
}

TEST(MenuBarMenuTest, AddItemsBreaksClearReuseAndSpacing)
{
	spk::MenuBar::Menu menu("Menu");
	menu.addItem("open", "Open");
	menu.addBreak("separator");
	menu.addBreak("styled-separator", defaultStyle().menuBreak.get());
	EXPECT_EQ(menu.nbElement(), 3u);
	menu.setElementSpacing(11);
	EXPECT_EQ(menu.elementSpacing(), 11u);
	menu.clear();
	EXPECT_EQ(menu.nbElement(), 0u);
	menu.addItem("again", "Again");
	EXPECT_EQ(menu.nbElement(), 1u);
}

TEST(MenuBarMenuTest, BackgroundAccessorsAreStable)
{
	spk::MenuBar::Menu menu("Menu");
	const auto &constant = static_cast<const spk::MenuBar::Menu &>(menu);
	EXPECT_EQ(&constant.background(), &menu.background());
}

TEST(MenuBarMenuRenderTest, DISABLED_StandaloneWithSeparator)
{
	spk::MenuBar::Menu menu("Menu");
	menu.addItem("open", "Open");
	menu.addBreak("separator", defaultStyle().menuBreak.get());
	menu.addItem("quit", "Quit");
	menu.setGeometry({.anchor = {50, 50}, .size = {220, 160}});
	menu.activate();
	expectWidgetImage(menu, "ui/widget/menu_bar", "standalone_menu_with_separator");
}
