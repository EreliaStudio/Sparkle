#include "rendering/render_snapshot.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"
#include "ui/widget/interface_window.hpp"
#include <filesystem>
#include <gtest/gtest.h>

namespace
{
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

TEST(InterfaceWindowTemplateTest, OwnsTypedContentObject)
{
	spk::InterfaceWindow<spk::TextLabel> window("Window");
	window.contentObject().setText("Body");
	EXPECT_EQ(window.content(), &window.contentObject());
	EXPECT_EQ(window.contentObject().parent(), &window.normalBackground());
	const auto &constant = static_cast<const spk::InterfaceWindow<spk::TextLabel> &>(window);
	EXPECT_EQ(&constant.contentObject(), &window.contentObject());
}

TEST(InterfaceWindowTemplateRenderTest, DISABLED_Normal)
{
	spk::InterfaceWindow<spk::TextLabel> window("Window");
	window.setTitle("Inspector");
	window.contentObject().setText("Window content");
	window.contentObject().activate();
	window.setGeometry({.anchor = {70, 50}, .size = {360, 240}});
	window.activate();
	expectWidgetImage(window, "ui/widget/interface_window", "normal");
}

TEST(InterfaceWindowTemplateRenderTest, DISABLED_Minimized)
{
	spk::InterfaceWindow<spk::TextLabel> window("Window");
	window.setTitle("Inspector");
	window.setGeometry({.anchor = {70, 50}, .size = {360, 240}});
	window.minimize();
	window.activate();
	expectWidgetImage(window, "ui/widget/interface_window", "minimized");
}

TEST(InterfaceWindowTemplateRenderTest, DISABLED_CustomPaddingAndMenu)
{
	spk::InterfaceWindow<spk::TextLabel> window("Window");
	window.setTitle("Custom");
	window.contentObject().setText("Padded body");
	window.contentObject().activate();
	window.setMenuHeight(38);
	window.setContentPadding({.left = 20, .right = 12, .top = 10, .bottom = 18});
	window.setMenuButtonEnabled(spk::IInterfaceWindow::MenuBar::Button::Maximize, false);
	window.setGeometry({.anchor = {50, 40}, .size = {420, 270}});
	window.activate();
	expectWidgetImage(window, "ui/widget/interface_window", "custom_padding_menu");
}
