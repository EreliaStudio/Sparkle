#include <gtest/gtest.h>

#include "graphics/image.hpp"
#include "ui/widget/interface_window.hpp"
#include "ui/widget/scroll_bar.hpp"
#include "ui/widget/text_edit.hpp"
#include "ui/widget/toggle_switch.hpp"
#include "ui/widget/workspace.hpp"

TEST(WidgetStyleReactionTest, EveryScalarFieldReappliesToItsConsumerAndChangesHints)
{
	spk::Widget::Style style;
	style.textLabelTextSize = {26, 2};
	style.textLabelGlyphColor = {1, 0, 0, 1};
	style.textLabelOutlineColor = {0, 1, 0, 1};
	style.pushButtonCornerSize = {13, 11};
	style.pushButtonTextSize = {28, 1};
	style.pushButtonReleasedGlyphColor = {1, 1, 0, 1};
	style.pushButtonPressedGlyphColor = {0, 1, 1, 1};
	style.pushButtonAlignment = {spk::Alignment::Horizontal::Right, spk::Alignment::Vertical::Bottom};
	style.iconButtonIconSize = {31, 29};
	style.iconButtonIconPadding = {9, 8};
	style.textEditCornerSize = {10, 11};
	style.textEditTextSize = {29, 2};
	style.textEditGlyphColor = {1, 0, 1, 1};
	style.textEditOutlineColor = {0, 1, 1, 1};
	style.textEditCursorColor = {0, 0, 1, 1};
	style.sliderBarBackgroundCornerSize = {7, 6};
	style.sliderBarBodyCornerSize = {9, 10};
	style.scrollBarButtonIconSize = {21, 23};
	style.interfaceWindowBackgroundCornerSize = {15, 16};
	style.interfaceWindowMenuTitleTextSize = {27, 2};
	style.interfaceWindowMenuTitleGlyphColor = {1, 0, 0, 1};
	style.interfaceWindowMenuTitleOutlineColor = {0, 0, 1, 1};
	style.interfaceWindowMenuTitleAlignment = {spk::Alignment::Horizontal::Right, spk::Alignment::Vertical::Bottom};
	style.interfaceWindowMenuButtonIconSize = {17, 18};
	style.interfaceWindowMenuButtonIconPadding = {6, 7};
	style.interfaceWindowMenuButtonCornerSize = {4, 5};
	spk::TextLabel label("Label");
	label.setText("Measured");
	const auto oldHint = label.minimalSize();
	label.applyStyle(style);
	EXPECT_NE(label.minimalSize(), oldHint);
	EXPECT_EQ(label.textSize(), style.textLabelTextSize);
	EXPECT_EQ(label.glyphColor(), style.textLabelGlyphColor);
	EXPECT_EQ(label.outlineColor(), style.textLabelOutlineColor);
	spk::IconButton button("Button");
	button.setText("Button");
	button.applyStyle(style);
	EXPECT_EQ(button.releasedBackground().cornerSize(), style.pushButtonCornerSize);
	EXPECT_EQ(button.pressedBackground().cornerSize(), style.pushButtonCornerSize);
	EXPECT_EQ(button.releasedLabel().textSize(), style.pushButtonTextSize);
	EXPECT_EQ(button.pressedLabel().textSize(), style.pushButtonTextSize);
	EXPECT_EQ(button.releasedLabel().glyphColor(), style.pushButtonReleasedGlyphColor);
	EXPECT_EQ(button.pressedLabel().glyphColor(), style.pushButtonPressedGlyphColor);
	EXPECT_EQ(button.alignment(), style.pushButtonAlignment);
	EXPECT_EQ(button.iconSize(), style.iconButtonIconSize);
	EXPECT_EQ(button.iconPadding(), style.iconButtonIconPadding);
	spk::TextEdit edit("Edit");
	edit.applyStyle(style);
	EXPECT_EQ(edit.cornerSize(), style.textEditCornerSize);
	EXPECT_EQ(edit.textSize(), style.textEditTextSize);
	EXPECT_EQ(edit.glyphColor(), style.textEditGlyphColor);
	EXPECT_EQ(edit.outlineColor(), style.textEditOutlineColor);
	EXPECT_EQ(edit.cursorColor(), style.textEditCursorColor);
	spk::ScrollBar scroll("Scroll");
	scroll.applyStyle(style);
	EXPECT_EQ(scroll.slider().background().cornerSize(), style.sliderBarBackgroundCornerSize);
	EXPECT_EQ(scroll.slider().body().cornerSize(), style.sliderBarBodyCornerSize);
	EXPECT_EQ(scroll.negativeButton().iconSize(), style.scrollBarButtonIconSize);
	EXPECT_EQ(scroll.positiveButton().iconSize(), style.scrollBarButtonIconSize);
	spk::IInterfaceWindow window("Window");
	window.applyStyle(style);
	EXPECT_EQ(window.normalBackground().cornerSize(), style.interfaceWindowBackgroundCornerSize);
	EXPECT_EQ(window.minimizedBackground().cornerSize(), style.interfaceWindowBackgroundCornerSize);
	EXPECT_EQ(window.menuBar().titleLabel().textSize(), style.interfaceWindowMenuTitleTextSize);
	EXPECT_EQ(window.menuBar().titleLabel().glyphColor(), style.interfaceWindowMenuTitleGlyphColor);
	EXPECT_EQ(window.menuBar().titleLabel().outlineColor(), style.interfaceWindowMenuTitleOutlineColor);
	EXPECT_EQ(window.menuBar().titleLabel().alignment(), style.interfaceWindowMenuTitleAlignment);
	for (auto *control : {&window.menuBar().minimizeButton(), &window.menuBar().maximizeButton(), &window.menuBar().closeButton()})
	{
		EXPECT_EQ(control->iconSize(), style.interfaceWindowMenuButtonIconSize);
		EXPECT_EQ(control->iconPadding(), style.interfaceWindowMenuButtonIconPadding);
		EXPECT_EQ(control->releasedBackground().cornerSize(), style.interfaceWindowMenuButtonCornerSize);
		EXPECT_EQ(control->pressedBackground().cornerSize(), style.interfaceWindowMenuButtonCornerSize);
	}
}

TEST(WidgetStyleReactionTest, ResourceReplacementReachesComposedConsumers)
{
	const auto root = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path().parent_path().parent_path() / "resources";
	const auto sheet = [&](const char *file, spk::Vector2UInt grid = {3, 3}) {
		return std::make_unique<spk::SpriteSheet>(spk::SpriteSheet::open(root / "textures" / file, grid));
	};
	spk::Widget::Style style;
	style.font = std::make_unique<spk::Font>(root / "fonts/arial.ttf");
	style.iconsetImage = std::make_unique<spk::Image>(spk::Image::open(root / "textures/default_iconset.png"));
	style.iconset = sheet("default_iconset.png", {10, 10});
	style.nineSlice = sheet("default_nine_slice.png");
	style.darkNineSlice = sheet("default_nine_slice_dark.png");
	style.darkerNineSlice = sheet("default_nine_slice_darker.png");
	style.sliderBody = sheet("default_slider_body.png");
	style.menuBreak = sheet("default_break.png", {3, 1});
	style.toggleSwitchOutline = sheet("default_toggle_switch_nine_slice.png");
	style.toggleSwitchThumb = sheet("default_toggle_switch_thumb_nine_slice.png");
	style.toggleSwitchOffBackground = sheet("default_toggle_switch_off_background.png");
	style.toggleSwitchOnBackground = sheet("default_toggle_switch_on_background.png");
	spk::Panel panel("Panel");
	panel.applyStyle(style);
	EXPECT_EQ(panel.spriteSheet(), style.nineSlice.get());
	spk::ImageLabel image("Image");
	image.applyStyle(style);
	EXPECT_EQ(image.texture(), style.iconsetImage.get());
	spk::IconButton button("Button");
	button.applyStyle(style);
	EXPECT_EQ(button.iconset(), style.iconset.get());
	EXPECT_EQ(button.releasedLabel().font(), style.font.get());
	EXPECT_EQ(button.releasedBackground().spriteSheet(), style.darkNineSlice.get());
	EXPECT_EQ(button.pressedBackground().spriteSheet(), style.darkerNineSlice.get());
	spk::SliderBar slider("Slider");
	slider.applyStyle(style);
	EXPECT_EQ(slider.body().spriteSheet(), style.sliderBody.get());
	spk::MenuBar::Menu::Break separator("Break");
	separator.applyStyle(style);
	EXPECT_EQ(separator.spriteSheet(), style.menuBreak.get());
	spk::ToggleSwitch toggle("Toggle");
	toggle.applyStyle(style);
	EXPECT_EQ(toggle.outlineTexture(), style.toggleSwitchOutline.get());
	EXPECT_EQ(toggle.thumbTexture(), style.toggleSwitchThumb.get());
	EXPECT_EQ(toggle.uncheckedBackgroundTexture(), style.toggleSwitchOffBackground.get());
	EXPECT_EQ(toggle.checkedBackgroundTexture(), style.toggleSwitchOnBackground.get());
}

// Workspace currently inherits Widget::applyStyle, whose implementation is a no-op.
TEST(WorkspaceTest, DISABLED_ApplyStylePropagatesToContentAndMenus)
{
	spk::Workspace<spk::TextLabel> workspace("Workspace");
	workspace.menuBar().addMenu("file", "File");
	spk::Widget::Style style;
	style.textLabelTextSize = {31, 2};
	style.pushButtonTextSize = {30, 1};
	workspace.applyStyle(style);
	EXPECT_EQ(workspace.content().textSize(), style.textLabelTextSize);
	EXPECT_EQ(workspace.menuBar().button("file").releasedLabel().textSize(), style.pushButtonTextSize);
}
