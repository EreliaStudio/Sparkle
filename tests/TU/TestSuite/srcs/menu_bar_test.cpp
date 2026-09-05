#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "rendering/render_snapshot.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"
#include "ui/widget/menu_bar.hpp"

namespace
{
	void expectWidgetImage(spk::Widget &widget, const std::filesystem::path &category, const std::string &name)
	{
		auto &context = sparkle_test::OpenGLTestContext::instance();
		context.reset();

		spk::RenderSnapshot::Builder builder;
		widget.buildRenderSnapshot(builder);
		spk::RenderSnapshot snapshot = builder.build();
		snapshot.execute(context.renderContext());

		const auto actual = sparkle_test::resultImagePath(category, name);
		const auto expected = sparkle_test::expectedImagePath(category, name);
		const auto difference = sparkle_test::resultImagePath(category, name + "_difference");
		context.save(actual);

		ASSERT_TRUE(std::filesystem::exists(expected))
			<< "Missing golden image: " << expected << "\n"
			<< "The current render was saved to: " << actual;

		const sparkle_test::ImageComparisonResult result =
			sparkle_test::compareImages(actual, expected, difference);
		EXPECT_TRUE(result.matches)
			<< "Image mismatch for [" << category.string() << "/" << name << "]\n"
			<< "Different pixels: " << result.differentPixelCount << "\n"
			<< "Actual size: " << result.actualWidth << "x" << result.actualHeight << "\n"
			<< "Expected size: " << result.expectedWidth << "x" << result.expectedHeight << "\n"
			<< "Difference image: " << difference;
	}

	[[nodiscard]] const spk::Widget::Style &defaultStyle()
	{
		return spk::Widget::defaultStyle.get();
	}
}

TEST(MenuBarTest, DefaultState)
{
	spk::MenuBar bar("MenuBar");
	EXPECT_EQ(bar.nbMenu(), 0u);
	EXPECT_EQ(bar.height(), 25u);
	EXPECT_EQ(bar.contentInset(), 2u);
	EXPECT_EQ(bar.menuSpacing(), 5u);
}

TEST(MenuBarTest, AddLookupAndConfigureMenus)
{
	spk::MenuBar bar("MenuBar");
	auto &file = bar.addMenu("file", "File");
	auto &edit = bar.addMenu("edit", "Edit");
	file.addItem("open", "Open");
	edit.addItem("copy", "Copy");
	EXPECT_EQ(bar.nbMenu(), 2u);
	EXPECT_EQ(&bar.menu("file"), &file);
	EXPECT_NE(&bar.button("file"), nullptr);
	const spk::MenuBar &constant = bar;
	EXPECT_EQ(&constant.menu("edit"), &edit);
	EXPECT_EQ(&constant.button("file"), &bar.button("file"));
}

TEST(MenuBarTest, MissingMenuAndButtonThrow)
{
	spk::MenuBar bar("MenuBar");
	EXPECT_THROW(bar.menu("missing"), std::out_of_range);
	EXPECT_THROW(bar.button("missing"), std::out_of_range);
}

TEST(MenuBarTest, GeometryConfigurationRoundTrips)
{
	spk::MenuBar bar("MenuBar");
	bar.setHeight(41);
	bar.setContentInset(7);
	bar.setMenuSpacing(13);
	EXPECT_EQ(bar.height(), 41u);
	EXPECT_EQ(bar.contentInset(), 7u);
	EXPECT_EQ(bar.menuSpacing(), 13u);
}

TEST(MenuBarTest, ApplyStyleCloseMenusAndBackgroundAccessors)
{
	spk::MenuBar bar("MenuBar");
	bar.applyStyle(defaultStyle());
	bar.addMenu("file", "File").addItem("open", "Open");
	EXPECT_NO_THROW(bar.closeMenus());
	const spk::MenuBar &constant = bar;
	EXPECT_EQ(&constant.background(), &bar.background());
}

TEST(MenuBarTest, DuplicateMenuNameIsRejected)
{
	spk::MenuBar bar("MenuBar");
	bar.addMenu("file", "File");
	EXPECT_THROW(bar.addMenu("file", "Duplicate"), std::invalid_argument);
}

TEST(MenuBarRenderTest, DISABLED_ClosedMenus)
{
	spk::MenuBar bar("MenuBar");
	bar.addMenu("file", "File").addItem("open", "Open");
	bar.addMenu("edit", "Edit").addItem("copy", "Copy");
	bar.setGeometry({.anchor = {20, 20}, .size = {500, 35}});
	bar.activate();
	expectWidgetImage(bar, "ui/widget/menu_bar", "closed_menus");
}

TEST(MenuBarRenderTest, DISABLED_CustomDimensions)
{
	spk::MenuBar bar("MenuBar");
	bar.addMenu("file", "File");
	bar.addMenu("tools", "Tools");
	bar.setHeight(48);
	bar.setContentInset(8);
	bar.setMenuSpacing(16);
	bar.setGeometry({.anchor = {20, 20}, .size = {520, 48}});
	bar.activate();
	expectWidgetImage(bar, "ui/widget/menu_bar", "custom_dimensions");
}
