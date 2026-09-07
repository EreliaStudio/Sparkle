#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

#include "core/context/update_context.hpp"
#include "rendering/render_snapshot.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"
#include "ui/widget/message_box.hpp"
#include "ui/widget/numeric_spin_box.hpp"
#include "ui/widget/panel.hpp"
#include "ui/widget/popup_widget.hpp"
#include "ui/widget/progress_bar.hpp"
#include "ui/widget/prompt_panel.hpp"
#include "ui/widget/push_button.hpp"
#include "ui/widget/radio_button.hpp"
#include "ui/widget/scroll_area.hpp"
#include "ui/widget/slider_bar.hpp"
#include "ui/widget/spin_box.hpp"
#include "ui/widget/tab_widget.hpp"
#include "ui/widget/text_area.hpp"
#include "ui/widget/text_edit.hpp"
#include "ui/widget/text_label.hpp"
#include "ui/widget/toggle_switch.hpp"
#include "ui/widget/tooltip.hpp"
#include "ui/widget/workspace.hpp"

namespace
{
	void expectWidgetImage(spk::Widget &widget, const std::string &name, const std::string &state = "standard")
	{
		auto &context = sparkle_test::OpenGLTestContext::instance();
		context.reset();

		spk::RenderSnapshot::Builder builder;
		widget.buildRenderSnapshot(builder);
		builder.build().execute(context.renderContext());

		const std::filesystem::path category = std::filesystem::path("ui/widget") / name;
		const auto actual = sparkle_test::resultImagePath(category, state);
		const auto expected = sparkle_test::expectedImagePath(category, state);
		const auto difference = sparkle_test::resultImagePath(category, state + "_difference");
		context.save(actual);

		ASSERT_TRUE(std::filesystem::exists(expected))
			<< "Missing golden image: " << expected << "\n"
			<< "The current render was saved to: " << actual;

		const auto result = sparkle_test::compareImages(actual, expected, difference);
		EXPECT_TRUE(result.matches)
			<< "Image mismatch for [" << category.string() << "]\n"
			<< "Different pixels: " << result.differentPixelCount << "\n"
			<< "Actual image: " << actual << "\n"
			<< "Expected image: " << expected << "\n"
			<< "Difference image: " << difference;
	}

	void movePointer(spk::Widget &widget, const spk::Vector2Int &position)
	{
		spk::Mouse mouse;
		mouse.position = position;
		spk::MouseMovedRecord record{};
		record.position = position;
		spk::MouseMovedEvent event(record, mouse);
		widget.dispatch(event);
	}

	void pressPointer(spk::Widget &widget, const spk::Vector2Int &position)
	{
		spk::Mouse mouse;
		mouse.position = position;
		spk::MouseButtonPressedRecord record{};
		record.button = spk::Mouse::Button::Left;
		spk::MouseButtonPressedEvent event(record, mouse);
		widget.dispatch(event);
	}

	void advanceWidget(spk::Widget &widget, std::chrono::milliseconds duration)
	{
		spk::Keyboard keyboard;
		spk::Mouse mouse;
		spk::UpdateContext context{.time = {}, .deltaTime = duration, .keyboard = keyboard, .mouse = mouse};
		widget.updateState(context);
	}
}

TEST(WidgetGoldenRenderTest, Panel)
{
	spk::Panel widget("Panel");
	widget.setGeometry({.anchor = {20, 20}, .size = {180, 100}});
	widget.setCornerSize({12, 12});
	expectWidgetImage(widget, "panel");
}

TEST(WidgetGoldenRenderStateTest, PanelCornerAndGeometryVariants)
{
	spk::Panel widget("Panel");
	widget.setGeometry({.anchor = {20, 20}, .size = {180, 100}});
	widget.setCornerSize({0, 0});
	expectWidgetImage(widget, "panel", "square_corners");
	widget.setCornerSize({8, 8});
	expectWidgetImage(widget, "panel", "small_corners");
	widget.setCornerSize({24, 12});
	expectWidgetImage(widget, "panel", "asymmetric_corners");
	widget.setGeometry({.anchor = {20, 20}, .size = {320, 48}});
	expectWidgetImage(widget, "panel", "wide_geometry");
}

TEST(WidgetGoldenRenderTest, TextLabel)
{
	spk::TextLabel widget("TextLabel");
	widget.setGeometry({.anchor = {20, 20}, .size = {260, 80}});
	widget.setText(U"Sparkle — Héllo 世界");
	widget.setTextSize({24, 1});
	widget.setPadding({12, 8});
	widget.setAlignment({spk::Alignment::Horizontal::Left, spk::Alignment::Vertical::Center});
	expectWidgetImage(widget, "text_label");
}

TEST(WidgetGoldenRenderStateTest, TextLabelAlignmentMatrix)
{
	struct AlignmentCase
	{
		const char *name;
		spk::Alignment alignment;
	};
	const AlignmentCase cases[] = {
		{"top_left", {spk::Alignment::Horizontal::Left, spk::Alignment::Vertical::Top}},
		{"top_center", {spk::Alignment::Horizontal::Center, spk::Alignment::Vertical::Top}},
		{"top_right", {spk::Alignment::Horizontal::Right, spk::Alignment::Vertical::Top}},
		{"center_left", {spk::Alignment::Horizontal::Left, spk::Alignment::Vertical::Center}},
		{"center", {spk::Alignment::Horizontal::Center, spk::Alignment::Vertical::Center}},
		{"center_right", {spk::Alignment::Horizontal::Right, spk::Alignment::Vertical::Center}},
		{"bottom_left", {spk::Alignment::Horizontal::Left, spk::Alignment::Vertical::Bottom}},
		{"bottom_center", {spk::Alignment::Horizontal::Center, spk::Alignment::Vertical::Bottom}},
		{"bottom_right", {spk::Alignment::Horizontal::Right, spk::Alignment::Vertical::Bottom}},
	};
	spk::TextLabel widget("TextLabel");
	widget.setGeometry({.anchor = {20, 20}, .size = {280, 100}});
	widget.setText("Aligned label");
	widget.setPadding({12, 10});
	for (const auto &entry : cases)
	{
		widget.setAlignment(entry.alignment);
		expectWidgetImage(widget, "text_label", entry.name);
	}
	widget.setText("");
	expectWidgetImage(widget, "text_label", "empty");
}

TEST(WidgetGoldenRenderTest, TextArea)
{
	spk::TextArea widget("TextArea");
	widget.setGeometry({.anchor = {20, 20}, .size = {260, 130}});
	widget.setText("A wrapped text area with two lines\nand stable golden rendering.");
	widget.setTextSize({20, 1});
	widget.setLinePadding(5);
	widget.setAlignment({spk::Alignment::Horizontal::Left, spk::Alignment::Vertical::Top});
	expectWidgetImage(widget, "text_area");
}

TEST(WidgetGoldenRenderStateTest, TextAreaWrappingAndAlignmentVariants)
{
	spk::TextArea widget("TextArea");
	widget.setText("A long sentence that wraps differently depending on the available rendering width.");
	widget.setTextSize({18, 1});
	widget.setLinePadding(4);
	widget.setGeometry({.anchor = {20, 20}, .size = {170, 150}});
	widget.setAlignment({spk::Alignment::Horizontal::Left, spk::Alignment::Vertical::Top});
	expectWidgetImage(widget, "text_area", "narrow_wrapped_top_left");
	widget.setGeometry({.anchor = {20, 20}, .size = {380, 100}});
	expectWidgetImage(widget, "text_area", "wide_wrapped_top_left");
	widget.setGeometry({.anchor = {20, 20}, .size = {280, 150}});
	widget.setAlignment({spk::Alignment::Horizontal::Center, spk::Alignment::Vertical::Center});
	expectWidgetImage(widget, "text_area", "centered");
	widget.setAlignment({spk::Alignment::Horizontal::Right, spk::Alignment::Vertical::Bottom});
	expectWidgetImage(widget, "text_area", "bottom_right");
	widget.setText("Explicit first line\nSecond line\nThird line");
	expectWidgetImage(widget, "text_area", "explicit_lines");
}

TEST(WidgetGoldenRenderTest, PushButton)
{
	spk::PushButton widget("PushButton");
	widget.setGeometry({.anchor = {20, 20}, .size = {200, 56}});
	widget.setText("Launch");
	widget.setIcon(spk::Widget::defaultStyle->iconset.get(), 1);
	widget.setIconSize({24, 24});
	expectWidgetImage(widget, "push_button");
}

TEST(WidgetGoldenRenderStateTest, PushButtonContentAndPointerStates)
{
	const auto render = [](std::string_view state, std::string_view text, bool withIcon, bool flat, bool hovered, bool pressed) {
		spk::PushButton widget("PushButton");
		widget.setGeometry({.anchor = {20, 20}, .size = {200, 56}});
		widget.setText(text);
		if (withIcon)
		{
			widget.setIcon(spk::Widget::defaultStyle->iconset.get(), 1);
			widget.setIconSize({24, 24});
		}
		widget.setFlat(flat);
		if (hovered)
			movePointer(widget, {30, 30});
		if (pressed)
			pressPointer(widget, {30, 30});
		expectWidgetImage(widget, "push_button", std::string(state));
	};

	render("text_only_released", "Launch", false, false, false, false);
	render("text_only_hovered", "Launch", false, false, true, false);
	render("text_only_pressed", "Launch", false, false, true, true);
	render("icon_only_released", "", true, false, false, false);
	render("icon_only_hovered", "", true, false, true, false);
	render("icon_only_pressed", "", true, false, true, true);
	render("text_and_icon_hovered", "Launch", true, false, true, false);
	render("text_and_icon_pressed", "Launch", true, false, true, true);
	render("empty_released", "", false, false, false, false);
	render("flat_released", "Launch", false, true, false, false);
	render("flat_pressed", "Launch", false, true, true, true);
}

TEST(WidgetGoldenRenderTest, RadioButton)
{
	spk::RadioButton widget("RadioButton");
	widget.setGeometry({.anchor = {20, 20}, .size = {220, 52}});
	widget.setText("Selected option");
	widget.setChecked(true);
	expectWidgetImage(widget, "radio_button");
}

TEST(WidgetGoldenRenderStateTest, RadioButtonStandaloneAndGroupSelections)
{
	{
		spk::RadioButton widget("RadioButton");
		widget.setGeometry({.anchor = {20, 20}, .size = {220, 52}});
		widget.setText("Standalone option");
		expectWidgetImage(widget, "radio_button", "standalone_unchecked");
		pressPointer(widget, {30, 30});
		expectWidgetImage(widget, "radio_button", "standalone_pressed");
	}

	spk::RadioButtonGroup group("RadioButtonGroup");
	group.setSpacing(8);
	group.setIndicatorSize({20, 20});
	group.setElementPadding({18, 12});
	group.insert(0, 0, "Small");
	group.insert(1, 0, "Medium");
	group.insert(0, 1, "Large");
	group.insert(1, 1, "Extra large");
	group.setGeometry({.anchor = {20, 20}, .size = {420, 140}});

	group.select(0, 0);
	expectWidgetImage(group, "radio_button_group", "selected_top_left");
	group.select(1, 0);
	expectWidgetImage(group, "radio_button_group", "selected_top_right");
	group.select(0, 1);
	expectWidgetImage(group, "radio_button_group", "selected_bottom_left");
	group.select(1, 1);
	expectWidgetImage(group, "radio_button_group", "selected_bottom_right");
	group.setAllowNoSelection(true);
	group.clearSelection();
	expectWidgetImage(group, "radio_button_group", "no_selection");
}

TEST(WidgetGoldenRenderTest, ToggleSwitch)
{
	spk::ToggleSwitch widget("ToggleSwitch");
	widget.setAnimationDuration({});
	widget.setChecked(true);
	widget.setGeometry({.anchor = {20, 20}, .size = {100, 48}});
	expectWidgetImage(widget, "toggle_switch");
}

TEST(WidgetGoldenRenderStateTest, ToggleSwitchOrientationsAndTransition)
{
	spk::ToggleSwitch horizontal("HorizontalToggle");
	horizontal.setGeometry({.anchor = {20, 20}, .size = {100, 48}});
	horizontal.setAnimationDuration({});
	expectWidgetImage(horizontal, "toggle_switch", "horizontal_off");
	horizontal.setChecked(true);
	expectWidgetImage(horizontal, "toggle_switch", "horizontal_on");

	spk::ToggleSwitch transition("TransitionToggle");
	transition.setGeometry({.anchor = {20, 20}, .size = {100, 48}});
	transition.setAnimationDuration(std::chrono::milliseconds(100));
	transition.setChecked(true);
	advanceWidget(transition, std::chrono::milliseconds(50));
	expectWidgetImage(transition, "toggle_switch", "horizontal_half_transition");

	spk::ToggleSwitch vertical("VerticalToggle");
	vertical.setOrientation(spk::Orientation::Vertical);
	vertical.setGeometry({.anchor = {20, 20}, .size = {48, 100}});
	vertical.setAnimationDuration({});
	expectWidgetImage(vertical, "toggle_switch", "vertical_off");
	vertical.setChecked(true);
	expectWidgetImage(vertical, "toggle_switch", "vertical_on");
}

TEST(WidgetGoldenRenderTest, ProgressBar)
{
	spk::ProgressBar widget("ProgressBar");
	widget.setGeometry({.anchor = {20, 20}, .size = {260, 36}});
	widget.setRatio(0.68f);
	expectWidgetImage(widget, "progress_bar");
}

TEST(WidgetGoldenRenderTest, LabeledProgressBar)
{
	spk::LabeledProgressBar widget("LabeledProgressBar");
	widget.setGeometry({.anchor = {20, 20}, .size = {260, 40}});
	widget.setRatio(0.42f);
	expectWidgetImage(widget, "labeled_progress_bar");
}

TEST(WidgetGoldenRenderStateTest, ProgressBarBoundaryAndIntermediateRatios)
{
	spk::ProgressBar progress("ProgressBar");
	progress.setGeometry({.anchor = {20, 20}, .size = {260, 36}});
	for (const auto &[state, ratio] : {std::pair{"empty", 0.0f}, std::pair{"quarter", 0.25f}, std::pair{"half", 0.5f}, std::pair{"three_quarters", 0.75f}, std::pair{"full", 1.0f}})
	{
		progress.setRatio(ratio);
		expectWidgetImage(progress, "progress_bar", state);
	}

	spk::LabeledProgressBar labeled("LabeledProgressBar");
	labeled.setGeometry({.anchor = {20, 20}, .size = {260, 40}});
	labeled.setRatio(0.0f);
	expectWidgetImage(labeled, "labeled_progress_bar", "empty");
	labeled.setRatio(0.5f);
	expectWidgetImage(labeled, "labeled_progress_bar", "half");
	labeled.setRatio(1.0f);
	expectWidgetImage(labeled, "labeled_progress_bar", "full");
}

TEST(WidgetGoldenRenderStateTest, ProgressBarFillDirections)
{
	spk::ProgressBar widget("ProgressBar");
	widget.setGeometry({.anchor = {20, 20}, .size = {260, 100}});
	widget.setRatio(0.6f);
	for (const auto &[state, direction] : {
			 std::pair{"left_to_right", spk::ProgressBar::FillDirection::LeftToRight},
			 std::pair{"right_to_left", spk::ProgressBar::FillDirection::RightToLeft},
			 std::pair{"bottom_to_top", spk::ProgressBar::FillDirection::BottomToTop},
			 std::pair{"top_to_bottom", spk::ProgressBar::FillDirection::TopToBottom}})
	{
		widget.setFillDirection(direction);
		expectWidgetImage(widget, "progress_bar", state);
	}
}

TEST(WidgetGoldenRenderTest, SliderBar)
{
	spk::SliderBar widget("SliderBar");
	widget.setGeometry({.anchor = {20, 20}, .size = {260, 32}});
	widget.setScale(0.22f);
	widget.setRatio(0.65f);
	expectWidgetImage(widget, "slider_bar");
}

TEST(WidgetGoldenRenderStateTest, SliderBarOrientationsAndPositions)
{
	spk::SliderBar horizontal("HorizontalSlider");
	horizontal.setGeometry({.anchor = {20, 20}, .size = {260, 32}});
	horizontal.setScale(0.2f);
	for (const auto &[state, ratio] : {std::pair{"horizontal_start", 0.0f}, std::pair{"horizontal_center", 0.5f}, std::pair{"horizontal_end", 1.0f}})
	{
		horizontal.setRatio(ratio);
		expectWidgetImage(horizontal, "slider_bar", state);
	}

	spk::SliderBar vertical("VerticalSlider");
	vertical.setOrientation(spk::Orientation::Vertical);
	vertical.setGeometry({.anchor = {20, 20}, .size = {32, 260}});
	vertical.setScale(0.2f);
	for (const auto &[state, ratio] : {std::pair{"vertical_start", 0.0f}, std::pair{"vertical_center", 0.5f}, std::pair{"vertical_end", 1.0f}})
	{
		vertical.setRatio(ratio);
		expectWidgetImage(vertical, "slider_bar", state);
	}
}

TEST(WidgetGoldenRenderTest, ScrollBar)
{
	spk::ScrollBar widget("ScrollBar");
	widget.setOrientation(spk::Orientation::Vertical);
	widget.setGeometry({.anchor = {20, 20}, .size = {32, 240}});
	widget.setScale(0.3f);
	widget.setRatio(0.55f);
	expectWidgetImage(widget, "scroll_bar");
}

TEST(WidgetGoldenRenderStateTest, ScrollBarOrientationsAndPositions)
{
	spk::ScrollBar horizontal("HorizontalScrollBar");
	horizontal.setOrientation(spk::Orientation::Horizontal);
	horizontal.setGeometry({.anchor = {20, 20}, .size = {280, 32}});
	horizontal.setScale(0.25f);
	horizontal.setRatio(0.0f);
	expectWidgetImage(horizontal, "scroll_bar", "horizontal_start");
	horizontal.setRatio(1.0f);
	expectWidgetImage(horizontal, "scroll_bar", "horizontal_end");

	spk::ScrollBar vertical("VerticalScrollBar");
	vertical.setOrientation(spk::Orientation::Vertical);
	vertical.setGeometry({.anchor = {20, 20}, .size = {32, 280}});
	vertical.setScale(0.25f);
	vertical.setRatio(0.0f);
	expectWidgetImage(vertical, "scroll_bar", "vertical_start");
	vertical.setRatio(1.0f);
	expectWidgetImage(vertical, "scroll_bar", "vertical_end");
}

TEST(WidgetGoldenRenderTest, ScrollArea)
{
	spk::ScrollArea<spk::TextArea> widget("ScrollArea");
	widget.setGeometry({.anchor = {20, 20}, .size = {260, 140}});
	widget.contentObject().setText("Oversized scrollable content\nwith multiple lines\nand both scroll bars visible.");
	widget.contentObject().setMinimalSize({420, 260});
	expectWidgetImage(widget, "scroll_area");
}

TEST(WidgetGoldenRenderStateTest, ScrollAreaScrollbarVisibilityMatrix)
{
	spk::ScrollArea<spk::TextArea> widget("ScrollArea");
	widget.setGeometry({.anchor = {20, 20}, .size = {280, 160}});
	widget.contentObject().setText("Scrollable content");
	widget.contentObject().setMinimalSize({120, 80});
	expectWidgetImage(widget, "scroll_area", "no_scrollbars");
	widget.contentObject().setMinimalSize({480, 80});
	expectWidgetImage(widget, "scroll_area", "horizontal_scrollbar");
	widget.contentObject().setMinimalSize({120, 320});
	expectWidgetImage(widget, "scroll_area", "vertical_scrollbar");
	widget.contentObject().setMinimalSize({480, 320});
	widget.horizontalScrollBar().setRatio(0.35f);
	widget.verticalScrollBar().setRatio(0.6f);
	expectWidgetImage(widget, "scroll_area", "both_scrollbars_scrolled");
}

TEST(WidgetGoldenRenderTest, TextEdit)
{
	spk::TextEdit widget("TextEdit");
	widget.setGeometry({.anchor = {20, 20}, .size = {260, 44}});
	widget.setPlaceholder("Enter a value...");
	widget.setText("Editable text");
	expectWidgetImage(widget, "text_edit");
}

TEST(WidgetGoldenRenderStateTest, TextEditContentStates)
{
	spk::TextEdit widget("TextEdit");
	widget.setGeometry({.anchor = {20, 20}, .size = {280, 44}});
	widget.setPlaceholder("Enter a value...");
	expectWidgetImage(widget, "text_edit", "placeholder");
	widget.setText("Editable text");
	expectWidgetImage(widget, "text_edit", "populated");
	widget.setSelection(0, 8);
	expectWidgetImage(widget, "text_edit", "selected_text");
	widget.clearSelection();
	widget.setObscured(true);
	expectWidgetImage(widget, "text_edit", "obscured");
	widget.setObscured(false);
	widget.disableEdit();
	expectWidgetImage(widget, "text_edit", "editing_disabled");
}

TEST(WidgetGoldenRenderTest, SpinBox)
{
	spk::SpinBox<int> widget("SpinBox");
	widget.setGeometry({.anchor = {20, 20}, .size = {220, 44}});
	widget.setMinimum(-100);
	widget.setMaximum(100);
	widget.setValue(42);
	expectWidgetImage(widget, "spin_box");
}

TEST(WidgetGoldenRenderStateTest, SpinBoxValuesAndLimits)
{
	spk::SpinBox<int> widget("SpinBox");
	widget.setGeometry({.anchor = {20, 20}, .size = {220, 44}});
	widget.setMinimum(-100);
	widget.setMaximum(100);
	widget.setValue(-100);
	expectWidgetImage(widget, "spin_box", "minimum");
	widget.setValue(0);
	expectWidgetImage(widget, "spin_box", "zero");
	widget.setValue(100);
	expectWidgetImage(widget, "spin_box", "maximum");
}

TEST(WidgetGoldenRenderTest, NumericSpinBox)
{
	spk::NumericSpinBox<double> widget("NumericSpinBox");
	widget.setGeometry({.anchor = {20, 20}, .size = {220, 44}});
	widget.setValue(42.5);
	expectWidgetImage(widget, "numeric_spin_box");
}

TEST(WidgetGoldenRenderStateTest, NumericSpinBoxSignedAndFloatingValues)
{
	spk::NumericSpinBox<int> integer("IntegerSpinBox");
	integer.setGeometry({.anchor = {20, 20}, .size = {220, 44}});
	integer.setValue(-42);
	expectWidgetImage(integer, "numeric_spin_box", "negative_integer");
	integer.setValue(0);
	expectWidgetImage(integer, "numeric_spin_box", "zero_integer");

	spk::NumericSpinBox<double> floating("FloatingSpinBox");
	floating.setGeometry({.anchor = {20, 20}, .size = {260, 44}});
	floating.setValue(123.75);
	expectWidgetImage(floating, "numeric_spin_box", "positive_float");
}

TEST(WidgetGoldenRenderTest, TabWidget)
{
	spk::TabWidget widget("TabWidget");
	spk::TextLabel first("FirstPage", &widget);
	spk::TextLabel second("SecondPage", &widget);
	first.setText("First page content");
	second.setText("Second page content");
	widget.addPage("First", &first);
	widget.addPage("Second", &second);
	widget.selectPage(1);
	widget.setGeometry({.anchor = {20, 20}, .size = {360, 180}});
	expectWidgetImage(widget, "tab_widget");
}

TEST(WidgetGoldenRenderTest, PromptPanel)
{
	spk::PromptPanel widget("PromptPanel");
	widget.setMessage("Do you want to continue with this operation?");
	widget.addButton("cancel", "Cancel");
	widget.addButton("continue", "Continue");
	widget.setGeometry({.anchor = {20, 20}, .size = {360, 180}});
	expectWidgetImage(widget, "prompt_panel");
}

TEST(WidgetGoldenRenderTest, MessageBox)
{
	spk::MessageBox widget("MessageBox");
	widget.setTitle("Confirmation");
	widget.setText("The operation completed successfully.");
	widget.addButton("close", "Close");
	widget.setGeometry({.anchor = {20, 20}, .size = {400, 240}});
	expectWidgetImage(widget, "message_box");
}

TEST(WidgetGoldenRenderTest, PopupWidget)
{
	spk::Widget root("Root", nullptr);
	root.setGeometry({.anchor = {0, 0}, .size = {420, 260}});
	spk::PushButton anchor("Anchor", &root);
	anchor.setText("Open menu");
	anchor.setGeometry({.anchor = {40, 32}, .size = {140, 44}});
	spk::PopupWidget popup("Popup", &root);
	spk::PromptPanel content("Content", &popup);
	content.setMessage("Choose an action");
	content.addButton("cancel", "Cancel");
	content.addButton("accept", "Accept");
	content.setPreferredSize({260, 140});
	popup.setContent(&content);
	popup.setAnchorWidget(&anchor);
	popup.open();
	expectWidgetImage(root, "popup_widget");
}

TEST(WidgetGoldenRenderTest, Tooltip)
{
	spk::Widget root("Root", nullptr);
	root.setGeometry({.anchor = {0, 0}, .size = {420, 220}});
	spk::PushButton target("Target", &root);
	target.setText("Hover target");
	target.setGeometry({.anchor = {48, 100}, .size = {160, 44}});
	spk::Tooltip tooltip("Tooltip", &root);
	tooltip.setTarget(&target);
	tooltip.setText("Helpful tooltip text");
	tooltip.setPlacement(spk::Tooltip::Placement::AboveTarget);
	tooltip.show();
	expectWidgetImage(root, "tooltip");
}

TEST(WidgetGoldenRenderTest, Workspace)
{
	spk::Workspace<spk::TextArea> widget("Workspace");
	widget.content().setText("Workspace content");
	widget.content().setAlignment({spk::Alignment::Horizontal::Center, spk::Alignment::Vertical::Center});
	widget.menuBar().addMenu("file", "File");
	widget.menuBar().addMenu("edit", "Edit");
	widget.setGeometry({.anchor = {20, 20}, .size = {440, 260}});
	expectWidgetImage(widget, "workspace");
}
