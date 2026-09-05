#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "rendering/render_snapshot.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"
#include "ui/widget/icon_button.hpp"

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

TEST(IconButtonTest, DefaultState)
{
	spk::IconButton button("Icon");
	EXPECT_EQ(button.iconSpriteID(), 0u);
}

TEST(IconButtonTest, ConstructorStoresIconsetAndSpriteId)
{
	const auto *sheet = defaultStyle().iconset.get();
	spk::IconButton button("Icon", sheet, 4);
	EXPECT_EQ(button.iconset(), sheet);
	EXPECT_EQ(button.iconSpriteID(), 4u);
}

TEST(IconButtonTest, ApplyStyleUsesDefaultIconset)
{
	spk::IconButton button("Icon");
	button.applyStyle(defaultStyle());
	EXPECT_EQ(button.iconset(), defaultStyle().iconset.get());
}

TEST(IconButtonTest, SetIconsetAndSpriteIdRoundTrip)
{
	spk::IconButton button("Icon");
	button.setIconset(defaultStyle().iconset.get());
	button.setIconSpriteID(17);
	EXPECT_EQ(button.iconset(), defaultStyle().iconset.get());
	EXPECT_EQ(button.iconSpriteID(), 17u);
}

TEST(IconButtonTest, CoordinateSetterResolvesSpriteId)
{
	spk::IconButton button("Icon", defaultStyle().iconset.get());
	button.setIconSpriteID(spk::Vector2UInt{3, 2});
	EXPECT_EQ(button.iconSpriteID(), defaultStyle().iconset->spriteID({3, 2}));
}

TEST(IconButtonTest, InvalidIconInputsPropagate)
{
	spk::IconButton button("Icon");
	EXPECT_THROW(button.setIconset(nullptr), std::invalid_argument);
	button.setIconset(defaultStyle().iconset.get());
	EXPECT_THROW(button.setIconSpriteID(defaultStyle().iconset->nbSprite()), std::out_of_range);
}

TEST(IconButtonTest, DISABLED_CoordinateResolutionWithoutIconsetThrows)
{
	// No public API can currently create an IconButton without the default-style iconset.
	// Keep this contract visible until such construction or iconset removal is supported.
}

TEST(IconButtonTest, ClickUsesInheritedPushButtonContract)
{
	spk::IconButton button("Icon", defaultStyle().iconset.get(), 0);
	button.setGeometry({.anchor = {0, 0}, .size = {80, 40}});
	button.activate();
	int calls = 0;
	auto contract = button.subscribeToClick([&] {
		++calls;
	});
	spk::Mouse mouse;
	mouse.position = {10, 10};
	spk::MouseButtonPressedRecord pressedRecord{};
	pressedRecord.button = spk::Mouse::Button::Left;
	spk::MouseButtonPressedEvent pressed(pressedRecord, mouse);
	button.dispatch(pressed);
	spk::MouseButtonReleasedRecord releasedRecord{};
	releasedRecord.button = spk::Mouse::Button::Left;
	spk::MouseButtonReleasedEvent released(releasedRecord, mouse);
	button.dispatch(released);
	EXPECT_EQ(calls, 1);
}

TEST(IconButtonRenderTest, DISABLED_DefaultSprite)
{
	spk::IconButton button("Icon", defaultStyle().iconset.get(), 0);
	button.setGeometry({.anchor = {80, 70}, .size = {120, 80}});
	button.activate();
	expectWidgetImage(button, "ui/widget/icon_button", "default_sprite");
}

TEST(IconButtonRenderTest, DISABLED_AlternateSprite)
{
	spk::IconButton button("Icon", defaultStyle().iconset.get(), 22);
	button.setGeometry({.anchor = {80, 70}, .size = {120, 80}});
	button.activate();
	expectWidgetImage(button, "ui/widget/icon_button", "alternate_sprite");
}

TEST(IconButtonRenderTest, DISABLED_LargeGeometry)
{
	spk::IconButton button("Icon", defaultStyle().iconset.get(), 8);
	button.setGeometry({.anchor = {40, 40}, .size = {260, 130}});
	button.activate();
	expectWidgetImage(button, "ui/widget/icon_button", "large_geometry");
}
