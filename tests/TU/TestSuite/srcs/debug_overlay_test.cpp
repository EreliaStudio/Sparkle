#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "rendering/render_snapshot.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"
#include "ui/widget/debug_overlay.hpp"
#include "ui/widget/text_label.hpp"

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

TEST(DebugOverlayTest, DefaultStateIsEmpty)
{
	spk::DebugOverlay overlay("Overlay");
	EXPECT_EQ(overlay.rowCount(), 0u);
	EXPECT_EQ(overlay.columnCount(), 0u);
}

TEST(DebugOverlayTest, SetWidgetExpandsGridAndReturnsElement)
{
	spk::DebugOverlay overlay("Overlay");
	spk::TextLabel first("First", &overlay);
	spk::TextLabel second("Second", &overlay);
	EXPECT_NE(overlay.setWidget(0, 0, &first), nullptr);
	EXPECT_NE(overlay.setWidget(2, 1, &second), nullptr);
	EXPECT_EQ(overlay.columnCount(), 3u);
	EXPECT_EQ(overlay.rowCount(), 2u);
	EXPECT_EQ(overlay.widget(0, 0), &first);
	EXPECT_EQ(overlay.widget(2, 1), &second);
}

TEST(DebugOverlayTest, SetWidgetAcceptsExplicitSizeSettings)
{
	spk::DebugOverlay overlay("Overlay");
	spk::TextLabel label("Label", &overlay);
	spk::Layout::SizeSettings settings{spk::Layout::SizePolicy::Extend, spk::Layout::SizePolicy::Minimum};
	EXPECT_NE(overlay.setWidget(1, 1, &label, settings), nullptr);
	EXPECT_EQ(overlay.widget(1, 1), &label);
}

TEST(DebugOverlayTest, ClearCellAndClearRemoveMappings)
{
	spk::DebugOverlay overlay("Overlay");
	spk::TextLabel a("A", &overlay);
	spk::TextLabel b("B", &overlay);
	overlay.setWidget(0, 0, &a);
	overlay.setWidget(1, 0, &b);
	overlay.clearCell(0, 0);
	EXPECT_EQ(overlay.widget(0, 0), nullptr);
	EXPECT_EQ(overlay.widget(1, 0), &b);
	overlay.clear();
	EXPECT_EQ(overlay.widget(1, 0), nullptr);
}

TEST(DebugOverlayTest, PaddingRoundTrip)
{
	spk::DebugOverlay overlay("Overlay");
	overlay.setElementPadding({7, 11});
	EXPECT_EQ(overlay.elementPadding(), spk::Vector2UInt(7, 11));
}

TEST(DebugOverlayTest, ConstWidgetAccessorMatchesMutableAccessor)
{
	spk::DebugOverlay overlay("Overlay");
	spk::TextLabel label("Label", &overlay);
	overlay.setWidget(0, 0, &label);
	const spk::DebugOverlay &constant = overlay;
	EXPECT_EQ(constant.widget(0, 0), overlay.widget(0, 0));
}

TEST(DebugOverlayTest, RejectsNullForeignAndDuplicateWidgets)
{
	spk::DebugOverlay overlay("Overlay");
	spk::TextLabel foreign("Foreign");
	spk::TextLabel child("Child", &overlay);
	EXPECT_THROW(overlay.setWidget(0, 0, nullptr), std::invalid_argument);
	EXPECT_THROW(overlay.setWidget(0, 0, &foreign), std::invalid_argument);
	overlay.setWidget(0, 0, &child);
	EXPECT_THROW(overlay.setWidget(1, 0, &child), std::invalid_argument);
}

TEST(DebugOverlayTest, ReparentingAndReplacementRemoveOldMappings)
{
	spk::DebugOverlay overlay("Overlay");
	spk::Widget other("Other", nullptr);
	spk::TextLabel first("First", &overlay);
	spk::TextLabel second("Second", &overlay);
	overlay.setWidget(0, 0, &first);
	overlay.setWidget(0, 0, &second);
	EXPECT_EQ(overlay.widget(0, 0), &second);
	first.setParent(&other);
	EXPECT_EQ(overlay.widget(0, 0), &second);
	second.setParent(&other);
	EXPECT_EQ(overlay.widget(0, 0), nullptr);
	EXPECT_NO_THROW(overlay.clearCell(20, 20));
	EXPECT_NO_THROW(overlay.clear());
	EXPECT_NO_THROW(overlay.clear());
}

TEST(DebugOverlayRenderTest, DISABLED_SingleCell)
{
	spk::DebugOverlay overlay("Overlay");
	spk::TextLabel label("Label", &overlay);
	label.setText("FPS: 60");
	label.activate();
	overlay.setWidget(0, 0, &label);
	overlay.setGeometry({.anchor = {30, 30}, .size = {300, 80}});
	overlay.activate();
	expectWidgetImage(overlay, "ui/widget/debug_overlay", "single_cell");
}

TEST(DebugOverlayRenderTest, DISABLED_Grid)
{
	spk::DebugOverlay overlay("Overlay");
	spk::TextLabel a("A", &overlay);
	spk::TextLabel b("B", &overlay);
	spk::TextLabel c("C", &overlay);
	a.setText("FPS");
	b.setText("60");
	c.setText("16.6 ms");
	a.activate();
	b.activate();
	c.activate();
	overlay.setWidget(0, 0, &a);
	overlay.setWidget(1, 0, &b);
	overlay.setWidget(1, 1, &c);
	overlay.setElementPadding({6, 4});
	overlay.setGeometry({.anchor = {30, 30}, .size = {360, 150}});
	overlay.activate();
	expectWidgetImage(overlay, "ui/widget/debug_overlay", "grid");
}
