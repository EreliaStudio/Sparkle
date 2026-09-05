#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "rendering/render_snapshot.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"
#include "ui/widget/interface_window.hpp"

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

TEST(InterfaceWindowTest, DefaultState)
{
	spk::IInterfaceWindow window("Window");
	EXPECT_FALSE(window.isMinimized());
	EXPECT_FALSE(window.isMaximized());
	EXPECT_FALSE(window.isMoving());
	EXPECT_EQ(window.content(), nullptr);
	EXPECT_FALSE(window.contentPadding().has_value());
	EXPECT_EQ(window.minimumContentSize(), spk::Vector2(0.0f, 0.0f));
	EXPECT_EQ(window.menuHeight(), 20u);
}

TEST(InterfaceWindowTest, ContentTitleMenuHeightAndPaddingRoundTrip)
{
	spk::IInterfaceWindow window("Window");
	spk::TextLabel content("Content", &window.normalBackground());
	window.setContent(&content);
	window.setTitle("Settings");
	window.setMenuHeight(34);
	const spk::IInterfaceWindow::Padding padding{.left = 4, .right = 7, .top = 9, .bottom = 11};
	window.setContentPadding(padding);
	EXPECT_EQ(window.content(), &content);
	ASSERT_TRUE(window.contentPadding().has_value());
	EXPECT_EQ(*window.contentPadding(), padding);
	EXPECT_EQ(window.menuHeight(), 34u);
	window.resetContentPadding();
	EXPECT_FALSE(window.contentPadding().has_value());
}

TEST(InterfaceWindowTest, MinimumSizeAndMenuConfigurationApi)
{
	spk::IInterfaceWindow window("Window");
	window.setMinimumContentSize({120.0f, 80.0f});
	window.setMenuButtonEnabled(spk::IInterfaceWindow::MenuBar::Button::Minimize, false);
	window.setMaximizeSpriteIDs(7, 8);
	EXPECT_EQ(window.minimumContentSize(), spk::Vector2(120.0f, 80.0f));
	EXPECT_FALSE(window.menuBar().isButtonEnabled(spk::IInterfaceWindow::MenuBar::Button::Minimize));
}

TEST(InterfaceWindowTest, MinimizeAndMaximizeStateTransitions)
{
	spk::Widget parent("Parent", nullptr);
	parent.setGeometry({.anchor = {0, 0}, .size = {640, 480}});
	spk::IInterfaceWindow window("Window", &parent);
	window.setGeometry({.anchor = {40, 40}, .size = {320, 220}});
	window.minimize();
	EXPECT_TRUE(window.isMinimized());
	window.minimize();
	EXPECT_FALSE(window.isMinimized());
	window.maximize();
	EXPECT_TRUE(window.isMaximized());
	window.maximize();
	EXPECT_FALSE(window.isMaximized());
}

TEST(InterfaceWindowTest, CloseDeactivatesWindow)
{
	spk::IInterfaceWindow window("Window");
	window.activate();
	window.close();
	EXPECT_FALSE(window.isActive());
}

TEST(InterfaceWindowTest, ResizeSubscriptionReceivesContentSizeChanges)
{
	spk::IInterfaceWindow window("Window");
	std::vector<spk::Vector2UInt> sizes;
	auto contract = window.subscribeToResize([&](const spk::Vector2UInt &size) {
		sizes.push_back(size);
	});
	window.setGeometry({.anchor = {10, 10}, .size = {300, 200}});
	window.setGeometry({.anchor = {10, 10}, .size = {360, 240}});
	ASSERT_GE(sizes.size(), 2u);
	EXPECT_EQ(sizes.back(), spk::Vector2UInt(332, 200));
}

TEST(InterfaceWindowTest, ConstAndMutableAccessorsReferenceSameObjects)
{
	spk::IInterfaceWindow window("Window");
	const spk::IInterfaceWindow &constant = window;
	EXPECT_EQ(&constant.normalBackground(), &window.normalBackground());
	EXPECT_EQ(&constant.minimizedBackground(), &window.minimizedBackground());
	EXPECT_EQ(&constant.menuBar(), &window.menuBar());
	EXPECT_EQ(constant.content(), window.content());
}

TEST(InterfaceWindowTest, RejectsForeignContentAndForgetsReparentedContent)
{
	spk::IInterfaceWindow window("Window");
	spk::TextLabel foreign("Foreign");
	EXPECT_THROW(window.setContent(&foreign), std::invalid_argument);
	spk::TextLabel content("Content", &window.normalBackground());
	window.setContent(&content);
	content.setParent(nullptr);
	EXPECT_EQ(window.content(), nullptr);
}

TEST(InterfaceWindowTest, InvalidMenuButtonEnumThrows)
{
	spk::IInterfaceWindow window("Window");
	const auto invalid = static_cast<spk::IInterfaceWindow::MenuBar::Button>(999);
	EXPECT_THROW(window.setMenuButtonEnabled(invalid, true), std::logic_error);
	EXPECT_THROW((void)window.menuBar().isButtonEnabled(invalid), std::logic_error);
}

TEST(InterfaceWindowTest, CloseButtonInvokesCloseSubscription)
{
	spk::IInterfaceWindow window("Window");
	auto &button = window.menuBar().closeButton();
	button.setGeometry({.anchor = {0, 0}, .size = {40, 30}});
	int calls = 0;
	auto contract = window.subscribeToClose([&] {
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
