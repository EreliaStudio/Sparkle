#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "rendering/render_snapshot.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"
#include "ui/widget/container_widget.hpp"
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

TEST(ContainerWidgetTest, DefaultState)
{
	spk::ContainerWidget container("Container");
	EXPECT_EQ(container.content(), nullptr);
	EXPECT_EQ(container.contentAnchor(), spk::Vector2Int(0, 0));
	EXPECT_EQ(container.contentSize(), spk::Vector2UInt(0, 0));
}

TEST(ContainerWidgetTest, ContentAndGeometrySettingsRoundTrip)
{
	spk::ContainerWidget container("Container");
	spk::TextLabel content("Content", &container);
	container.setContent(&content);
	container.setContentAnchor({12, 17});
	container.setContentSize({210, 90});
	EXPECT_EQ(container.content(), &content);
	EXPECT_EQ(container.contentAnchor(), spk::Vector2Int(12, 17));
	EXPECT_EQ(container.contentSize(), spk::Vector2UInt(210, 90));
	EXPECT_EQ(content.geometry().anchor, spk::Vector2Int(12, 17));
	EXPECT_EQ(content.geometry().size, spk::Vector2UInt(210, 90));
}

TEST(ContainerWidgetTest, ContentCanBeReplacedAndCleared)
{
	spk::ContainerWidget container("Container");
	spk::TextLabel first("First", &container);
	spk::TextLabel second("Second", &container);
	container.setContent(&first);
	container.setContent(&second);
	EXPECT_EQ(container.content(), &second);
	container.setContent(nullptr);
	EXPECT_EQ(container.content(), nullptr);
}

TEST(ContainerWidgetTest, ConstAccessorMatchesMutableAccessor)
{
	spk::ContainerWidget container("Container");
	spk::TextLabel child("Content", &container);
	container.setContent(&child);
	const spk::ContainerWidget &constant = container;
	EXPECT_EQ(constant.content(), container.content());
}

TEST(ContainerWidgetTest, RejectsContentThatIsNotAlreadyAChild)
{
	spk::ContainerWidget container("Container");
	spk::TextLabel foreign("Foreign");
	EXPECT_THROW(container.setContent(&foreign), std::invalid_argument);
	EXPECT_EQ(container.content(), nullptr);
}

TEST(ContainerWidgetTest, ReparentingContentClearsObservedPointer)
{
	spk::ContainerWidget container("Container");
	spk::Widget other("Other", nullptr);
	spk::TextLabel content("Content", &container);
	container.setContent(&content);
	content.setParent(&other);
	EXPECT_EQ(container.content(), nullptr);
}

TEST(ContainerWidgetTest, NegativeAnchorZeroAndOversizedContentArePreserved)
{
	spk::ContainerWidget container("Container");
	spk::TextLabel content("Content", &container);
	container.setContent(&content);
	container.setContentAnchor({-12, -7});
	container.setContentSize({0, 0});
	EXPECT_EQ(content.geometry().anchor, spk::Vector2Int(-12, -7));
	EXPECT_EQ(content.geometry().size, spk::Vector2UInt(0, 0));
	container.setContentSize({1000, 900});
	EXPECT_EQ(content.geometry().size, spk::Vector2UInt(1000, 900));
}

TEST(ContainerWidgetRenderTest, DISABLED_TextContent)
{
	spk::ContainerWidget container("Container");
	spk::TextLabel content("Content", &container);
	content.setText("Contained text");
	content.activate();
	container.setGeometry({.anchor = {50, 50}, .size = {340, 180}});
	container.setContent(&content);
	container.setContentAnchor({20, 30});
	container.setContentSize({250, 60});
	container.activate();
	expectWidgetImage(container, "ui/widget/container_widget", "text_content");
}

TEST(ContainerWidgetRenderTest, DISABLED_OffsetContent)
{
	spk::ContainerWidget container("Container");
	spk::TextLabel content("Content", &container);
	content.setText("Offset");
	content.activate();
	container.setGeometry({.anchor = {30, 30}, .size = {360, 220}});
	container.setContent(&content);
	container.setContentAnchor({110, 100});
	container.setContentSize({180, 70});
	container.activate();
	expectWidgetImage(container, "ui/widget/container_widget", "offset_content");
}
