#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "rendering/render_snapshot.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"
#include "ui/widget/command_panel.hpp"

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

TEST(CommandPanelTest, DefaultState)
{
	spk::CommandPanel panel("Commands");
	EXPECT_EQ(panel.nbButton(), 0u);
	EXPECT_EQ(panel.sizePolicy().horizontal, spk::Layout::SizePolicy::Minimum);
	EXPECT_EQ(panel.sizePolicy().vertical, spk::Layout::SizePolicy::Minimum);
}

TEST(CommandPanelTest, AddLookupAndRemoveButtons)
{
	spk::CommandPanel panel("Commands");
	auto &save = panel.addButton("save", "Save");
	auto &cancel = panel.addButton("cancel", "Cancel");
	EXPECT_EQ(panel.nbButton(), 2u);
	EXPECT_EQ(&panel.button("save"), &save);
	EXPECT_EQ(&panel.button("cancel"), &cancel);
	const spk::CommandPanel &constant = panel;
	EXPECT_EQ(&constant.button("save"), &save);
	panel.removeButton("save");
	EXPECT_EQ(panel.nbButton(), 1u);
	EXPECT_THROW(panel.button("save"), std::out_of_range);
}

TEST(CommandPanelTest, DuplicateButtonNameIsRejected)
{
	spk::CommandPanel panel("Commands");
	panel.addButton("same", "One");
	EXPECT_THROW(panel.addButton("same", "Two"), std::invalid_argument);
}

TEST(CommandPanelTest, SubscribeReturnsLiveClickContract)
{
	spk::CommandPanel panel("Commands");
	auto &button = panel.addButton("run", "Run");
	int calls = 0;
	auto contract = panel.subscribe("run", [&] {
		++calls;
	});
	button.setGeometry({.anchor = {0, 0}, .size = {80, 30}});
	button.activate();
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

TEST(CommandPanelTest, SizePolicyAndPaddingRoundTrip)
{
	spk::CommandPanel panel("Commands");
	spk::Layout::SizeSettings settings{spk::Layout::SizePolicy::Extend, spk::Layout::SizePolicy::Fixed};
	panel.setSizePolicy(settings);
	panel.setElementPadding({9, 7});
	EXPECT_EQ(panel.sizePolicy().horizontal, spk::Layout::SizePolicy::Extend);
	EXPECT_EQ(panel.sizePolicy().vertical, spk::Layout::SizePolicy::Fixed);
	EXPECT_EQ(panel.elementPadding(), spk::Vector2UInt(9, 7));
}

TEST(CommandPanelTest, RemoveThenReaddPreservesAUsableName)
{
	spk::CommandPanel panel("Commands");
	panel.addButton("run", "Run");
	panel.removeButton("run");
	auto &replacement = panel.addButton("run", "Run again");
	EXPECT_EQ(&panel.button("run"), &replacement);
	EXPECT_EQ(panel.nbButton(), 1u);
}

TEST(CommandPanelTest, MissingOperationsReportRequestedName)
{
	spk::CommandPanel panel("Commands");
	try
	{
		(void)panel.button("absent");
		FAIL() << "Expected missing lookup to throw";
	} catch (const std::out_of_range &error)
	{
		EXPECT_NE(std::string(error.what()).find("absent"), std::string::npos);
	}
	EXPECT_THROW((void)panel.subscribe("absent", [] {
	}),
				 std::out_of_range);
	EXPECT_THROW(panel.removeButton("absent"), std::out_of_range);
}

TEST(CommandPanelRenderTest, DISABLED_Empty)
{
	spk::CommandPanel panel("Commands");
	panel.setGeometry({.anchor = {40, 40}, .size = {360, 70}});
	panel.activate();
	expectWidgetImage(panel, "ui/widget/command_panel", "empty");
}

TEST(CommandPanelRenderTest, DISABLED_ThreeButtons)
{
	spk::CommandPanel panel("Commands");
	panel.addButton("apply", "Apply");
	panel.addButton("cancel", "Cancel");
	panel.addButton("help", "Help");
	panel.setGeometry({.anchor = {40, 40}, .size = {480, 70}});
	panel.activate();
	expectWidgetImage(panel, "ui/widget/command_panel", "three_buttons");
}

TEST(CommandPanelRenderTest, DISABLED_PaddedButtons)
{
	spk::CommandPanel panel("Commands");
	panel.addButton("yes", "Yes");
	panel.addButton("no", "No");
	panel.setElementPadding({14, 10});
	panel.setGeometry({.anchor = {40, 40}, .size = {360, 90}});
	panel.activate();
	expectWidgetImage(panel, "ui/widget/command_panel", "padded_buttons");
}
