#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "rendering/render_snapshot.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"
#include "ui/widget/checkable_icon_button.hpp"

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

TEST(CheckableIconButtonTest, DefaultConstructorUsesDocumentedIdsAndUncheckedState)
{
	spk::CheckableIconButton button("Checkable");
	EXPECT_FALSE(button.isChecked());
	EXPECT_EQ(button.uncheckedSpriteID(), spk::CheckableIconButton::DefaultUncheckedSpriteID);
	EXPECT_EQ(button.checkedSpriteID(), spk::CheckableIconButton::DefaultCheckedSpriteID);
}

TEST(CheckableIconButtonTest, ConstructorsAcceptIconsetAndExplicitIds)
{
	const auto *sheet = defaultStyle().iconset.get();
	spk::CheckableIconButton withSheet("Checkable", sheet);
	EXPECT_EQ(withSheet.uncheckedButton().iconset(), sheet);
	EXPECT_EQ(withSheet.checkedButton().iconset(), sheet);

	spk::CheckableIconButton explicitIds("Checkable", sheet, 3, 7);
	EXPECT_EQ(explicitIds.uncheckedSpriteID(), 3u);
	EXPECT_EQ(explicitIds.checkedSpriteID(), 7u);
}

TEST(CheckableIconButtonTest, ApplyStyleUpdatesInternalButtons)
{
	spk::CheckableIconButton button("Checkable");
	button.applyStyle(defaultStyle());
	EXPECT_EQ(button.uncheckedButton().iconset(), defaultStyle().iconset.get());
	EXPECT_EQ(button.checkedButton().iconset(), defaultStyle().iconset.get());
}

TEST(CheckableIconButtonTest, SetCheckedAndToggleNotifyStateSubscribers)
{
	spk::CheckableIconButton button("Checkable");
	std::vector<bool> states;
	auto contract = button.subscribeToState([&](bool state) {
		states.push_back(state);
	});
	button.setChecked(true);
	button.setChecked(true);
	button.toggle();
	ASSERT_EQ(states.size(), 2u);
	EXPECT_TRUE(states[0]);
	EXPECT_FALSE(states[1]);
	EXPECT_FALSE(button.isChecked());
}

TEST(CheckableIconButtonTest, TargetStateCallbackOnlyRunsForRequestedState)
{
	spk::CheckableIconButton button("Checkable");
	int checkedCount = 0;
	auto contract = button.addStateCallback(true, [&] {
		++checkedCount;
	});
	button.setChecked(true);
	button.setChecked(false);
	button.setChecked(true);
	EXPECT_EQ(checkedCount, 2);
}

TEST(CheckableIconButtonTest, SpriteSettersAcceptIdsAndCoordinates)
{
	spk::CheckableIconButton button("Checkable", defaultStyle().iconset.get());
	button.setUncheckedSpriteID(4);
	button.setCheckedSpriteID(5);
	EXPECT_EQ(button.uncheckedSpriteID(), 4u);
	EXPECT_EQ(button.checkedSpriteID(), 5u);
	button.setUncheckedSpriteID(spk::Vector2UInt{2, 1});
	button.setCheckedSpriteID(spk::Vector2UInt{3, 1});
	EXPECT_EQ(button.uncheckedSpriteID(), defaultStyle().iconset->spriteID({2, 1}));
	EXPECT_EQ(button.checkedSpriteID(), defaultStyle().iconset->spriteID({3, 1}));
}

TEST(CheckableIconButtonTest, ConstAndMutableChildAccessorsReferenceSameObjects)
{
	spk::CheckableIconButton button("Checkable");
	const spk::CheckableIconButton &constant = button;
	EXPECT_EQ(&constant.uncheckedButton(), &button.uncheckedButton());
	EXPECT_EQ(&constant.checkedButton(), &button.checkedButton());
}

TEST(CheckableIconButtonTest, StateActivatesOnlyMatchingChild)
{
	spk::CheckableIconButton button("Checkable");
	EXPECT_TRUE(button.uncheckedButton().isActive());
	EXPECT_FALSE(button.checkedButton().isActive());
	button.setChecked(true);
	EXPECT_FALSE(button.uncheckedButton().isActive());
	EXPECT_TRUE(button.checkedButton().isActive());
	button.deactivate();
	EXPECT_FALSE(button.isActive());
	EXPECT_FALSE(button.uncheckedButton().isActive());
	EXPECT_TRUE(button.checkedButton().isActive());
}

TEST(CheckableIconButtonTest, ClickingVisibleChildTogglesState)
{
	spk::CheckableIconButton button("Checkable", defaultStyle().iconset.get());
	button.setGeometry({.anchor = {0, 0}, .size = {80, 40}});
	spk::Mouse mouse;
	mouse.position = {10, 10};
	spk::MouseButtonPressedRecord pressedRecord{};
	pressedRecord.button = spk::Mouse::Button::Left;
	spk::MouseButtonPressedEvent pressed(pressedRecord, mouse);
	button.uncheckedButton().dispatch(pressed);
	spk::MouseButtonReleasedRecord releasedRecord{};
	releasedRecord.button = spk::Mouse::Button::Left;
	spk::MouseButtonReleasedEvent released(releasedRecord, mouse);
	button.uncheckedButton().dispatch(released);
	EXPECT_TRUE(button.isChecked());
}

TEST(CheckableIconButtonTest, InvalidSpriteCoordinatesPropagate)
{
	spk::CheckableIconButton button("Checkable", defaultStyle().iconset.get());
	EXPECT_THROW(button.setUncheckedSpriteID(defaultStyle().iconset->nbSprite()), std::out_of_range);
	EXPECT_THROW(button.setCheckedSpriteID(defaultStyle().iconset->nbSprite()), std::out_of_range);
}

TEST(CheckableIconButtonRenderTest, DISABLED_Unchecked)
{
	spk::CheckableIconButton button("Checkable", defaultStyle().iconset.get());
	button.setGeometry({.anchor = {90, 80}, .size = {96, 64}});
	button.activate();
	expectWidgetImage(button, "ui/widget/checkable_icon_button", "unchecked");
}

TEST(CheckableIconButtonRenderTest, DISABLED_Checked)
{
	spk::CheckableIconButton button("Checkable", defaultStyle().iconset.get());
	button.setGeometry({.anchor = {90, 80}, .size = {96, 64}});
	button.setChecked(true);
	button.activate();
	expectWidgetImage(button, "ui/widget/checkable_icon_button", "checked");
}

TEST(CheckableIconButtonRenderTest, DISABLED_AlternateSprites)
{
	spk::CheckableIconButton button("Checkable", defaultStyle().iconset.get(), 12, 13);
	button.setGeometry({.anchor = {90, 80}, .size = {128, 96}});
	button.setChecked(true);
	button.activate();
	expectWidgetImage(button, "ui/widget/checkable_icon_button", "alternate_sprites");
}

TEST(CheckableIconButtonRenderTest, AlternateUnchecked)
{
	spk::CheckableIconButton button("Checkable");
	button.setUncheckedSpriteID(1);
	button.setCheckedSpriteID(2);
	button.setChecked(false);
	button.setGeometry({.anchor = {40, 40}, .size = {80, 80}});
	expectWidgetImage(button, "ui/widget/checkable_icon_button", "alternate_unchecked");
}
