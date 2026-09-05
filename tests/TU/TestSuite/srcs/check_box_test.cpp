#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "rendering/render_snapshot.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"
#include "ui/widget/check_box.hpp"

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

TEST(CheckBoxTest, DefaultState)
{
	spk::CheckBox box("CheckBox");
	EXPECT_FALSE(box.isChecked());
	EXPECT_EQ(box.spacing(), 4u);
	EXPECT_EQ(box.indicatorSize(), spk::Vector2UInt(16, 16));
}

TEST(CheckBoxTest, AllConstructorsConfigureDependencies)
{
	const auto &style = defaultStyle();
	spk::CheckBox basic("Basic", style.iconset.get(), style.font.get());
	EXPECT_EQ(basic.indicator().uncheckedButton().iconset(), style.iconset.get());
	spk::CheckBox explicitIds("Explicit", style.iconset.get(), 4, 5, style.font.get());
	EXPECT_EQ(explicitIds.indicator().uncheckedSpriteID(), 4u);
	EXPECT_EQ(explicitIds.indicator().checkedSpriteID(), 5u);
}

TEST(CheckBoxTest, ApplyStyleUpdatesComposedChildren)
{
	spk::CheckBox box("CheckBox");
	box.applyStyle(defaultStyle());
	EXPECT_EQ(box.indicator().uncheckedButton().iconset(), defaultStyle().iconset.get());
}

TEST(CheckBoxTest, SetCheckedToggleAndSubscription)
{
	spk::CheckBox box("CheckBox");
	std::vector<bool> states;
	auto contract = box.subscribeToState([&](bool value) {
		states.push_back(value);
	});
	box.setChecked(true);
	box.setChecked(true);
	box.toggle();
	ASSERT_EQ(states.size(), 2u);
	EXPECT_TRUE(states[0]);
	EXPECT_FALSE(states[1]);
	EXPECT_FALSE(box.isChecked());
}

TEST(CheckBoxTest, TextOverloadsUpdateLabel)
{
	spk::CheckBox box("CheckBox");
	box.setText("UTF-8 label");
	EXPECT_EQ(box.label().text(), spk::Font::textFromUTF8("UTF-8 label"));
	const spk::Font::Text text = U"Wide label";
	box.setText(text);
	EXPECT_EQ(box.label().text(), text);
}

TEST(CheckBoxTest, SpacingAndIndicatorSizeRoundTrip)
{
	spk::CheckBox box("CheckBox");
	box.setSpacing(13);
	box.setIndicatorSize({31, 27});
	EXPECT_EQ(box.spacing(), 13u);
	EXPECT_EQ(box.indicatorSize(), spk::Vector2UInt(31, 27));
}

TEST(CheckBoxTest, ConstAndMutableAccessorsReferenceSameChildren)
{
	spk::CheckBox box("CheckBox");
	const spk::CheckBox &constant = box;
	EXPECT_EQ(&constant.indicator(), &box.indicator());
	EXPECT_EQ(&constant.label(), &box.label());
}

TEST(CheckBoxTest, LeftClickAnywhereInsideTogglesExactlyOnce)
{
	spk::CheckBox box("CheckBox");
	box.setGeometry({.anchor = {0, 0}, .size = {240, 40}});
	box.activate();
	std::vector<bool> states;
	auto contract = box.subscribeToState([&](bool state) {
		states.push_back(state);
	});
	spk::Mouse mouse;
	mouse.position = {200, 20};
	spk::MouseButtonPressedRecord pressedRecord{};
	pressedRecord.button = spk::Mouse::Button::Left;
	spk::MouseButtonPressedEvent pressed(pressedRecord, mouse);
	box.dispatch(pressed);
	spk::MouseButtonReleasedRecord releasedRecord{};
	releasedRecord.button = spk::Mouse::Button::Left;
	spk::MouseButtonReleasedEvent released(releasedRecord, mouse);
	box.dispatch(released);
	EXPECT_TRUE(box.isChecked());
	EXPECT_EQ(states, std::vector<bool>{true});
}

TEST(CheckBoxTest, ReleaseOutsideAndNonLeftButtonDoNotToggle)
{
	spk::CheckBox box("CheckBox");
	box.setGeometry({.anchor = {0, 0}, .size = {100, 40}});
	spk::Mouse mouse;
	mouse.position = {10, 10};
	spk::MouseButtonPressedRecord left{};
	left.button = spk::Mouse::Button::Left;
	spk::MouseButtonPressedEvent pressed(left, mouse);
	box.dispatch(pressed);
	mouse.position = {200, 200};
	spk::MouseButtonReleasedRecord releasedOutside{};
	releasedOutside.button = spk::Mouse::Button::Left;
	spk::MouseButtonReleasedEvent released(releasedOutside, mouse);
	box.dispatch(released);
	EXPECT_FALSE(box.isChecked());
	mouse.position = {10, 10};
	spk::MouseButtonPressedRecord right{};
	right.button = spk::Mouse::Button::Right;
	spk::MouseButtonPressedEvent rightPressed(right, mouse);
	box.dispatch(rightPressed);
	EXPECT_FALSE(rightPressed.consumed);
}

TEST(CheckBoxTest, InactiveCheckboxIgnoresClick)
{
	spk::CheckBox box("CheckBox");
	box.setGeometry({.anchor = {0, 0}, .size = {100, 40}});
	box.deactivate();
	spk::Mouse mouse;
	mouse.position = {10, 10};
	spk::MouseButtonPressedRecord record{};
	record.button = spk::Mouse::Button::Left;
	spk::MouseButtonPressedEvent event(record, mouse);
	box.dispatch(event);
	EXPECT_FALSE(event.consumed);
	EXPECT_FALSE(box.isChecked());
}

TEST(CheckBoxRenderTest, DISABLED_Unchecked)
{
	spk::CheckBox box("CheckBox");
	box.setText("Enable shadows");
	box.setGeometry({.anchor = {50, 60}, .size = {260, 48}});
	box.activate();
	expectWidgetImage(box, "ui/widget/check_box", "unchecked");
}

TEST(CheckBoxRenderTest, DISABLED_Checked)
{
	spk::CheckBox box("CheckBox");
	box.setText("Enable shadows");
	box.setChecked(true);
	box.setGeometry({.anchor = {50, 60}, .size = {260, 48}});
	box.activate();
	expectWidgetImage(box, "ui/widget/check_box", "checked");
}

TEST(CheckBoxRenderTest, DISABLED_CustomSpacingAndIndicatorSize)
{
	spk::CheckBox box("CheckBox");
	box.setText("Large indicator");
	box.setSpacing(18);
	box.setIndicatorSize({32, 32});
	box.setGeometry({.anchor = {50, 60}, .size = {300, 64}});
	box.activate();
	expectWidgetImage(box, "ui/widget/check_box", "custom_spacing_indicator");
}
