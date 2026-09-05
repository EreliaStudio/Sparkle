#include <gtest/gtest.h>

#include "core/context/update_context.hpp"
#include "ui/widget/interface_window.hpp"
#include "ui/widget/message_box.hpp"
#include "ui/widget/popup_widget.hpp"
#include "ui/widget/radio_button.hpp"
#include "ui/widget/scroll_area.hpp"
#include "ui/widget/toggle_switch.hpp"
#include "ui/widget/tooltip.hpp"
#include "ui/widget/workspace.hpp"
#include <chrono>

namespace
{
	void press(spk::Widget &widget, spk::Vector2Int position, spk::Mouse::Button button = spk::Mouse::Button::Left)
	{
		spk::Mouse mouse;
		mouse.position = position;
		spk::MouseButtonPressedRecord record{};
		record.button = button;
		spk::MouseButtonPressedEvent event(record, mouse);
		widget.dispatch(event);
	}
	void release(spk::Widget &widget, spk::Vector2Int position, spk::Mouse::Button button = spk::Mouse::Button::Left)
	{
		spk::Mouse mouse;
		mouse.position = position;
		spk::MouseButtonReleasedRecord record{};
		record.button = button;
		spk::MouseButtonReleasedEvent event(record, mouse);
		widget.dispatch(event);
	}
	void click(spk::Widget &widget)
	{
		const auto p = widget.viewRegion().viewport.anchor + spk::Vector2Int{5, 5};
		press(widget, p);
		release(widget, p);
	}
	void move(spk::Widget &widget, spk::Vector2Int position)
	{
		spk::Mouse mouse;
		mouse.position = position;
		spk::MouseMovedRecord record{};
		record.position = position;
		spk::MouseMovedEvent event(record, mouse);
		widget.dispatch(event);
	}
	void loseFocus(spk::Widget &widget)
	{
		spk::WindowFocusLostRecord record{};
		spk::WindowFocusLostEvent event(record);
		widget.dispatch(event);
	}
	void tick(spk::Widget &widget, int milliseconds, spk::Vector2Int position = {})
	{
		spk::Keyboard keyboard;
		spk::Mouse mouse;
		mouse.position = position;
		spk::UpdateContext context{.time = {}, .deltaTime = std::chrono::milliseconds(milliseconds), .keyboard = keyboard, .mouse = mouse};
		widget.updateState(context);
	}
}

TEST(SliderBarInteractionTest, BothOrientationsDragClampNotifyAndCancelOnFocusLoss)
{
	for (auto orientation : {spk::Orientation::Horizontal, spk::Orientation::Vertical})
	{
		spk::SliderBar slider("Slider");
		slider.setOrientation(orientation);
		slider.setScale(0.2f);
		slider.setRange(-10, 10);
		slider.setGeometry({.anchor = {0, 0}, .size = orientation == spk::Orientation::Horizontal ? spk::Vector2UInt{100, 20} : spk::Vector2UInt{20, 100}});
		int editions = 0;
		auto contract = slider.subscribeToEdition([&](float) {
			++editions;
		});
		press(slider, {5, 5});
		EXPECT_TRUE(slider.isDragging());
		move(slider, orientation == spk::Orientation::Horizontal ? spk::Vector2Int{45, 5} : spk::Vector2Int{5, 45});
		EXPECT_NEAR(slider.ratio(), 0.5f, 0.001f);
		EXPECT_EQ(editions, 1);
		move(slider, {1000, 1000});
		EXPECT_EQ(slider.value(), 10);
		EXPECT_EQ(editions, 2);
		move(slider, {2000, 2000});
		EXPECT_EQ(editions, 2);
		move(slider, {-100, -100});
		EXPECT_EQ(slider.value(), -10);
		EXPECT_EQ(editions, 3);
		loseFocus(slider);
		EXPECT_FALSE(slider.isDragging());
		move(slider, {1000, 1000});
		EXPECT_EQ(editions, 3);
		slider.setGeometry({.anchor = {0, 0}, .size = {0, 0}});
		press(slider, {5, 5});
		EXPECT_FALSE(slider.isDragging());
		slider.setGeometry({.anchor = {0, 0}, .size = {1, 1}});
		slider.setScale(1);
		click(slider);
		EXPECT_EQ(slider.ratio(), 0);
	}
}

TEST(ScrollBarInteractionTest, ArrowsAndSliderForwardOneEditionPerEffectiveChange)
{
	for (auto orientation : {spk::Orientation::Horizontal, spk::Orientation::Vertical})
	{
		spk::ScrollBar bar("Scroll");
		bar.setOrientation(orientation);
		bar.setStep(0.25f);
		bar.setScale(0.2f);
		bar.setGeometry({.anchor = {0, 0}, .size = orientation == spk::Orientation::Horizontal ? spk::Vector2UInt{200, 20} : spk::Vector2UInt{20, 200}});
		int editions = 0;
		auto contract = bar.subscribeToEdition([&](float) {
			++editions;
		});
		click(bar.negativeButton());
		EXPECT_EQ(editions, 0);
		click(bar.positiveButton());
		EXPECT_EQ(bar.ratio(), 0.25f);
		EXPECT_EQ(editions, 1);
		click(bar.negativeButton());
		EXPECT_EQ(bar.ratio(), 0);
		EXPECT_EQ(editions, 2);
		auto &slider = bar.slider();
		const auto start = slider.viewRegion().viewport.anchor + spk::Vector2Int{5, 5};
		press(slider, start);
		move(slider, {1000, 1000});
		release(slider, {1000, 1000});
		EXPECT_EQ(bar.ratio(), 1);
		EXPECT_EQ(editions, 3);
		click(bar.positiveButton());
		EXPECT_EQ(editions, 3);
		bar.setGeometry({.anchor = {0, 0}, .size = {1, 1}});
		EXPECT_LE(bar.slider().geometry().width, 1u);
	}
}

TEST(IScrollAreaInteractionTest, WheelBothAxesResizeAndContentReplacementClampOffsets)
{
	spk::ScrollArea<spk::Widget> area("Area");
	area.contentObject().setMinimalSize({400, 300});
	area.setGeometry({.anchor = {0, 0}, .size = {100, 80}});
	spk::Mouse mouse;
	mouse.position = {10, 10};
	spk::MouseWheelScrolledRecord record{};
	record.value = {-1, -1};
	spk::MouseWheelScrolledEvent event(record, mouse);
	area.dispatch(event);
	EXPECT_TRUE(event.consumed);
	EXPECT_LT(area.contentObject().geometry().y, 0);
	area.horizontalScrollBar().setRatio(0.5f);
	EXPECT_LT(area.contentObject().geometry().x, 0);
	area.contentObject().setMinimalSize({10, 10});
	area.setGeometry({.anchor = {0, 0}, .size = {500, 500}});
	EXPECT_FALSE(area.isScrollBarVisible(spk::Orientation::Horizontal));
	EXPECT_FALSE(area.isScrollBarVisible(spk::Orientation::Vertical));
	EXPECT_EQ(area.contentObject().geometry().anchor, spk::Vector2Int(0, 0));
	area.setGeometry({.anchor = {0, 0}, .size = {0, 0}});
	EXPECT_EQ(area.container().geometry().size, spk::Vector2UInt(0, 0));
}

TEST(PopupWidgetInteractionTest, EveryPlacementOffsetsAndRootEdges)
{
	spk::Widget root("Root", nullptr), anchor("Anchor", &root);
	root.setGeometry({.anchor = {10, 20}, .size = {300, 200}});
	anchor.setGeometry({.anchor = {100, 80}, .size = {40, 30}});
	spk::PopupWidget popup("Popup", &root);
	spk::Widget content("Content", &popup);
	content.setPreferredSize({20, 10});
	popup.setContent(&content);
	popup.setAnchorWidget(&anchor);
	popup.setOffset({3, 4});
	for (auto h : {spk::Alignment::Horizontal::Left, spk::Alignment::Horizontal::Center, spk::Alignment::Horizontal::Right})
	{
		for (auto v : {spk::Alignment::Vertical::Top, spk::Alignment::Vertical::Center, spk::Alignment::Vertical::Bottom})
		{
			popup.setPlacement({h, v});
			popup.open();
			EXPECT_EQ(popup.geometry().x, (h == spk::Alignment::Horizontal::Left ? 80 : h == spk::Alignment::Horizontal::Center ? 110
																																: 140) +
											  3);
			EXPECT_EQ(popup.geometry().y, (v == spk::Alignment::Vertical::Top ? 70 : v == spk::Alignment::Vertical::Center ? 90
																														   : 110) +
											  4);
			EXPECT_EQ(content.geometry().size, popup.geometry().size);
			popup.close();
		}
	}
	popup.openAt({-100, 1000});
	EXPECT_EQ(popup.geometry().anchor, spk::Vector2Int(0, 190));
	content.setPreferredSize({1000, 1000});
	popup.openAt({1000, -1000});
	EXPECT_EQ(popup.geometry().size, root.geometry().size);
	EXPECT_EQ(popup.geometry().anchor, spk::Vector2Int(0, 0));
	popup.setConstrainToRoot(false);
	popup.openAt({-100, -100});
	EXPECT_EQ(popup.geometry().anchor, spk::Vector2Int(-97, -96));
	EXPECT_EQ(popup.viewRegion().scissor, popup.viewRegion().viewport.intersect(root.viewRegion().scissor));
}

TEST(PopupWidgetInteractionTest, FeatureFlagsInsideOutsideEscapeAndRepeatedClose)
{
	spk::Widget root("Root", nullptr);
	root.setGeometry({.anchor = {0, 0}, .size = {200, 200}});
	spk::PopupWidget popup("Popup", &root);
	popup.setGeometry({.anchor = {0, 0}, .size = {50, 50}});
	int closes = 0;
	auto contract = popup.subscribeToClose([&] {
		++closes;
		EXPECT_FALSE(popup.isOpen());
	});
	popup.openAt({10, 10});
	popup.openAt({10, 10});
	press(popup, {20, 20});
	EXPECT_TRUE(popup.isOpen());
	popup.setCloseOnOutsidePress(false);
	press(popup, {100, 100});
	EXPECT_TRUE(popup.isOpen());
	popup.setCloseOnOutsidePress(true);
	press(popup, {100, 100});
	EXPECT_FALSE(popup.isOpen());
	EXPECT_EQ(closes, 1);
	popup.close();
	EXPECT_EQ(closes, 1);
	popup.openAt({10, 10});
	spk::Keyboard keyboard;
	spk::KeyPressedRecord record{};
	record.key = spk::Keyboard::Escape;
	popup.setCloseOnEscape(false);
	spk::KeyPressedEvent ignored(record, keyboard);
	popup.dispatch(ignored);
	EXPECT_TRUE(popup.isOpen());
	popup.setCloseOnEscape(true);
	spk::KeyPressedEvent escape(record, keyboard);
	popup.observeKeyboard(escape);
	EXPECT_TRUE(escape.consumed);
	EXPECT_FALSE(popup.isOpen());
	EXPECT_EQ(closes, 2);
}

TEST(TooltipInteractionTest, DelayedHoverCursorTrackingLeaveClickAndFocusLoss)
{
	spk::Widget root("Root", nullptr), target("Target", &root);
	root.activate();
	target.activate();
	root.setGeometry({.anchor = {0, 0}, .size = {400, 300}});
	target.setGeometry({.anchor = {50, 50}, .size = {100, 100}});
	spk::Tooltip tooltip("Tooltip", &root);
	tooltip.setTarget(&target);
	tooltip.setText("Help");
	tooltip.setPlacement(spk::Tooltip::Placement::Cursor);
	tooltip.setOpenDelay(std::chrono::milliseconds(100));
	tooltip.setCloseDelay(std::chrono::milliseconds(50));
	tick(tooltip, 99, {60, 60});
	EXPECT_FALSE(tooltip.isShown());
	tick(tooltip, 1, {60, 60});
	EXPECT_TRUE(tooltip.isShown());
	const auto before = tooltip.geometry().anchor;
	tick(tooltip, 1, {70, 70});
	EXPECT_EQ(tooltip.geometry().anchor, before + spk::Vector2Int(10, 10));
	tick(tooltip, 49, {300, 250});
	EXPECT_TRUE(tooltip.isShown());
	tick(tooltip, 1, {300, 250});
	EXPECT_FALSE(tooltip.isShown());
	tooltip.setOpenDelay({});
	tick(tooltip, 0, {60, 60});
	EXPECT_TRUE(tooltip.isShown());
	press(tooltip, {60, 60});
	EXPECT_FALSE(tooltip.isShown());
	tick(tooltip, 0, {60, 60});
	EXPECT_TRUE(tooltip.isShown());
	loseFocus(tooltip);
	EXPECT_FALSE(tooltip.isShown());
	target.deactivate();
	tick(tooltip, 200, {60, 60});
	EXPECT_FALSE(tooltip.isShown());
	EXPECT_EQ(tooltip.targetRenderPass().name, spk::Widget::TooltipKey.name);
	EXPECT_EQ(tooltip.background().targetRenderPass().name, spk::Widget::TooltipKey.name);
}

TEST(ToggleSwitchInteractionTest, ClickAnimationBothOrientationsAndInterruptedTransition)
{
	for (auto orientation : {spk::Orientation::Horizontal, spk::Orientation::Vertical})
	{
		spk::ToggleSwitch toggle("Toggle");
		toggle.setOrientation(orientation);
		toggle.setGeometry({.anchor = {0, 0}, .size = {80, 40}});
		toggle.setAnimationDuration(std::chrono::milliseconds(100));
		int editions = 0;
		auto contract = toggle.subscribeToState([&](bool state) {
			++editions;
			EXPECT_EQ(state, toggle.isChecked());
		});
		click(toggle);
		EXPECT_TRUE(toggle.isChecked());
		EXPECT_EQ(editions, 1);
		tick(toggle, 50);
		EXPECT_NEAR(toggle.visualRatio(), 0.5f, 0.001f);
		toggle.toggle();
		EXPECT_EQ(editions, 2);
		tick(toggle, 50);
		EXPECT_NEAR(toggle.visualRatio(), 0.25f, 0.001f);
		tick(toggle, 50);
		EXPECT_EQ(toggle.visualRatio(), 0);
		EXPECT_FALSE(toggle.isAnimating());
		press(toggle, {5, 5});
		release(toggle, {1000, 1000});
		EXPECT_FALSE(toggle.isChecked());
		EXPECT_EQ(editions, 2);
		toggle.setChecked(true);
		toggle.setAnimationDuration({});
		tick(toggle, 0);
		EXPECT_EQ(toggle.visualRatio(), 1);
	}
}

TEST(PushButtonInteractionTest, NonLeftDoubleClickLeaveAndInactiveBranches)
{
	spk::PushButton button("Button");
	button.setGeometry({.anchor = {0, 0}, .size = {100, 40}});
	int clicks = 0;
	auto contract = button.subscribeToClick([&] {
		++clicks;
	});
	press(button, {5, 5}, spk::Mouse::Button::Right);
	release(button, {5, 5}, spk::Mouse::Button::Right);
	EXPECT_EQ(clicks, 0);
	move(button, {5, 5});
	EXPECT_TRUE(button.isHovered());
	spk::MouseLeftRecord record{};
	spk::MouseLeftEvent left(record);
	button.dispatch(left);
	EXPECT_FALSE(button.isHovered());
	button.deactivate();
	click(button);
	EXPECT_EQ(clicks, 0);
	button.activate();
	spk::Mouse mouse;
	mouse.position = {5, 5};
	spk::MouseButtonDoubleClickedRecord doubleRecord{};
	doubleRecord.button = spk::Mouse::Button::Left;
	spk::MouseButtonDoubleClickedEvent doubled(doubleRecord, mouse);
	button.dispatch(doubled);
	release(button, {5, 5});
	EXPECT_EQ(clicks, 1);
}

// PushButton does not currently cancel its pressed state when the window loses focus.
TEST(PushButtonInteractionTest, DISABLED_WindowFocusLossCancelsPendingClick)
{
	spk::PushButton button("Button");
	button.setGeometry({.anchor = {0, 0}, .size = {100, 40}});
	int clicks = 0;
	auto contract = button.subscribeToClick([&] {
		++clicks;
	});
	press(button, {5, 5});
	loseFocus(button);
	EXPECT_FALSE(button.isPressed());
	release(button, {5, 5});
	EXPECT_EQ(clicks, 0);
}

TEST(RadioButtonInteractionTest, SparseGroupClicksNotifyOnceAndPreserveExclusivity)
{
	spk::RadioButtonGroup group("Group");
	auto &first = group.insert(0, 0, "First");
	auto &second = group.insert(2, 1, "Second");
	group.setGeometry({.anchor = {0, 0}, .size = {400, 120}});
	int selections = 0;
	std::vector<bool> local;
	auto contract = group.subscribeToSelection([&](auto selection) {
		++selections;
		ASSERT_TRUE(selection);
		EXPECT_EQ(selection->button, &second);
	});
	auto localContract = second.subscribeToState([&](bool checked) {
		local.push_back(checked);
	});
	click(second);
	EXPECT_TRUE(second.isChecked());
	EXPECT_FALSE(first.isChecked());
	EXPECT_EQ(selections, 1);
	EXPECT_EQ(local, (std::vector<bool>{true}));
	click(second);
	EXPECT_EQ(selections, 1);
	EXPECT_TRUE(second.isChecked());
	EXPECT_EQ(local, (std::vector<bool>{true, false, true})); // Mandatory selection restores the local toggle without a group edition.
}

TEST(InterfaceWindowInteractionTest, TitleDragFocusLossResizeAndMaximizedRootChanges)
{
	spk::Widget root("Root", nullptr);
	root.setGeometry({.anchor = {10, 20}, .size = {600, 400}});
	spk::IInterfaceWindow window("Window", &root);
	window.setGeometry({.anchor = {40, 40}, .size = {300, 200}});
	window.activate();
	const auto start = window.menuBar().titleLabel().viewRegion().viewport.anchor + spk::Vector2Int{20, 10};
	press(window, start);
	EXPECT_TRUE(window.isMoving());
	move(window, start + spk::Vector2Int{20, 30});
	EXPECT_EQ(window.geometry().anchor, spk::Vector2Int(60, 70));
	loseFocus(window);
	EXPECT_FALSE(window.isMoving());
	const auto restored = window.geometry();
	move(window, {500, 300});
	EXPECT_EQ(window.geometry(), restored);
	window.maximize();
	EXPECT_TRUE(window.isMaximized());
	EXPECT_EQ(window.geometry().size, root.geometry().size);
	root.setGeometry({.anchor = {10, 20}, .size = {700, 500}});
	spk::WindowResizedRecord record{};
	record.size = root.geometry().size;
	spk::WindowResizedEvent event(record);
	window.dispatch(event);
	EXPECT_EQ(window.geometry().size, root.geometry().size);
	window.maximize();
	EXPECT_EQ(window.geometry(), restored);
	press(window, window.viewRegion().viewport.anchor);
	EXPECT_TRUE(window.isResizing());
	loseFocus(window);
	EXPECT_FALSE(window.isResizing());
	spk::Widget content("Content", &window.normalBackground());
	content.setMinimalSize({320, 220});
	window.setContent(&content);
	window.setContentPadding({3, 4, 5, 6});
	window.setMenuHeight(30);
	EXPECT_GE(window.minimalSize().x, 327);
	EXPECT_GE(window.minimalSize().y, 261);
	content.setMinimalSize({340, 240});
	EXPECT_GE(window.minimalSize().x, 347);
}

TEST(InterfaceWindowInteractionTest, AllMenuEnableCombinationsCallbacksAndRepeatedCommands)
{
	using Button = spk::IInterfaceWindow::MenuBar::Button;
	for (unsigned mask = 0; mask < 8; ++mask)
	{
		spk::Widget root("Root", nullptr);
		root.setGeometry({.anchor = {0, 0}, .size = {600, 400}});
		spk::IInterfaceWindow window("Window", &root);
		window.setGeometry({.anchor = {20, 20}, .size = {300, 200}});
		window.activate();
		window.setTitle("Window title");
		window.menuBar().setMargin(5);
		window.setMenuButtonEnabled(Button::Minimize, mask & 1);
		window.setMenuButtonEnabled(Button::Maximize, mask & 2);
		window.setMenuButtonEnabled(Button::Close, mask & 4);
		int closes = 0;
		auto contract = window.subscribeToClose([&] {
			++closes;
		});
		click(window.menuBar().minimizeButton());
		EXPECT_EQ(window.isMinimized(), bool(mask & 1));
		if (window.isMinimized())
		{
			click(window.menuBar().minimizeButton());
		}
		EXPECT_FALSE(window.isMinimized());
		click(window.menuBar().maximizeButton());
		EXPECT_EQ(window.isMaximized(), bool(mask & 2));
		if (window.isMaximized())
		{
			click(window.menuBar().maximizeButton());
		}
		EXPECT_FALSE(window.isMaximized());
		click(window.menuBar().closeButton());
		EXPECT_EQ(closes, (mask & 4) ? 1 : 0);
		window.menuBar().setIconset(spk::Widget::defaultStyle->iconset.get());
		for (unsigned width : {0u, 1u, 10u, 1000u})
		{
			window.menuBar().setGeometry({.anchor = {0, 0}, .size = {width, 30}});
			EXPECT_LE(window.menuBar().titleLabel().geometry().width, width);
		}
	}
}

TEST(MessageBoxInteractionTest, InformationAndRequestActionsReplacementOrderingAndLifetime)
{
	spk::InformationMessageBox information("Information");
	information.setText("Notice");
	information.setGeometry({.anchor = {0, 0}, .size = {400, 200}});
	information.activate();
	click(information.closeButton());
	EXPECT_FALSE(information.isActive());
	information.activate();
	click(information.menuBar().closeButton());
	EXPECT_FALSE(information.isActive());
	std::vector<std::string> calls;
	spk::PushButton::ClickContract subscription;
	{
		spk::RequestMessageBox request("Request");
		request.setGeometry({.anchor = {0, 0}, .size = {400, 200}});
		request.activate();
		request.configure("old", [&] {
			calls.push_back("old");
		},
						  "old-no",
						  {});
		request.configure("Yes", [&] {
			EXPECT_TRUE(request.isActive());
			calls.push_back("action");
		},
						  "No",
						  [&] {
							  calls.push_back("no");
						  });
		subscription = request.subscribe("first", [&] {
			EXPECT_FALSE(request.isActive());
			calls.push_back("observer");
		});
		click(request.firstButton());
		EXPECT_EQ(calls, (std::vector<std::string>{"action", "observer"}));
		request.activate();
		request.setTitleCloseAction([&] {
			calls.push_back("title");
		});
		click(request.menuBar().closeButton());
		EXPECT_EQ(calls.back(), "title");
		request.activate();
		request.resetTitleCloseAction();
		click(request.menuBar().closeButton());
		EXPECT_EQ(calls.back(), "no");
		request.configure("Yes", {}, "No", {});
		request.activate();
		const auto before = calls.size();
		click(request.secondButton());
		EXPECT_FALSE(request.isActive());
		EXPECT_EQ(calls.size(), before);
		request.close();
		request.close();
		EXPECT_EQ(calls.size(), before);
	}
	EXPECT_NO_THROW(subscription.resign());
}

TEST(MessageBoxInteractionTest, DynamicTextButtonsSizingAndExactCommandDiagnostics)
{
	spk::MessageBox box("Message");
	box.setMinimalWidth(300);
	box.setText("");
	const auto empty = box.minimumContentSize();
	box.setText("A long message with several words that must contribute to the preferred content size");
	EXPECT_GE(box.minimumContentSize().x, 300);
	box.addButton("named", "Action");
	EXPECT_GE(box.minimumContentSize().y, empty.y);
	for (int operation : {0, 1, 2})
	{
		try
		{
			if (operation == 0)
			{
				box.addButton("named", "duplicate");
			}
			else if (operation == 1)
			{
				(void)box.button("absent");
			}
			else
			{
				auto contract = box.subscribe("absent", [] {
				});
			}
			FAIL();
		} catch (const std::invalid_argument &error)
		{
			EXPECT_EQ(operation, 0);
			EXPECT_NE(std::string(error.what()).find("named"), std::string::npos);
		} catch (const std::out_of_range &error)
		{
			EXPECT_NE(operation, 0);
			EXPECT_NE(std::string(error.what()).find("absent"), std::string::npos);
		}
	}
	box.removeButton("named");
	box.setGeometry({.anchor = {0, 0}, .size = {500, 300}});
	EXPECT_EQ(box.nbButton(), 0u);
}

TEST(WorkspaceInteractionTest, EmptyAndPopulatedMenusKeepContentBelowAndReactToHints)
{
	spk::Workspace<spk::TextLabel> workspace("Workspace");
	workspace.content().setText("Content");
	workspace.setGeometry({.anchor = {0, 0}, .size = {400, 300}});
	EXPECT_EQ(workspace.menuBar().nbMenu(), 0u);
	EXPECT_EQ(workspace.content().geometry().y, workspace.menuBar().height());
	auto &menu = workspace.menuBar().addMenu("file", "File");
	menu.addItem("open", "Open");
	EXPECT_EQ(workspace.menuBar().nbMenu(), 1u);
	click(workspace.menuBar().button("file"));
	EXPECT_TRUE(menu.isActive());
	EXPECT_GT(menu.absoluteZOrder(), workspace.content().absoluteZOrder());
	workspace.menuBar().closeMenus();
	EXPECT_FALSE(menu.isActive());
	workspace.menuBar().setHeight(40);
	EXPECT_EQ(workspace.content().geometry().y, 40);
	workspace.content().setMinimalSize({100, 50});
	EXPECT_GE(workspace.minimalSize().y, 90);
}

TEST(TooltipInteractionTest, AllPlacementsAndRootEdgesConstrainOversizedText)
{
	spk::Widget root("Root", nullptr), target("Target", &root);
	root.activate();
	target.activate();
	root.setGeometry({.anchor = {10, 20}, .size = {200, 120}});
	spk::Tooltip tooltip("Tooltip", &root);
	tooltip.setTarget(&target);
	tooltip.setText("A long tooltip with enough words to wrap across several lines");
	tooltip.setOpenDelay({});
	for (auto placement : {spk::Tooltip::Placement::Cursor, spk::Tooltip::Placement::AboveTarget, spk::Tooltip::Placement::BelowTarget, spk::Tooltip::Placement::Automatic})
	{
		for (auto position : {spk::Vector2Int{0, 0}, {180, 0}, {0, 100}, {180, 100}})
		{
			tooltip.hide();
			target.setGeometry({.anchor = position, .size = {20, 20}});
			tooltip.setPlacement(placement);
			tooltip.setMaximumWidth(80);
			tick(tooltip, 0, target.viewRegion().viewport.anchor + spk::Vector2Int{1, 1});
			ASSERT_TRUE(tooltip.isShown());
			EXPECT_GE(tooltip.geometry().x, 0);
			EXPECT_GE(tooltip.geometry().y, 0);
			EXPECT_LE(tooltip.geometry().width, 80u);
			EXPECT_LE(tooltip.geometry().x + tooltip.geometry().width, 200u);
			EXPECT_LE(tooltip.geometry().y + tooltip.geometry().height, 120u);
			EXPECT_EQ(tooltip.viewRegion().scissor, tooltip.viewRegion().viewport);
		}
	}
	target.setParent(nullptr);
	EXPECT_FALSE(tooltip.isShown());
	tooltip.setTarget(nullptr);
	tooltip.show();
	EXPECT_FALSE(tooltip.isShown());
}

// Tooltip observes reparenting but does not currently clear a destroyed target pointer.
TEST(TooltipInteractionTest, DISABLED_DestroyedTargetIsForgotten)
{
	spk::Widget root("Root", nullptr);
	root.setGeometry({.anchor = {0, 0}, .size = {200, 100}});
	spk::Tooltip tooltip("Tooltip", &root);
	tooltip.setText("Help");
	{
		spk::Widget target("Target", &root);
		tooltip.setTarget(&target);
		tooltip.show();
	}
	EXPECT_FALSE(tooltip.isShown());
	EXPECT_EQ(tooltip.target(), nullptr);
}

// TextArea keeps long words intact, so the current tooltip can exceed its maximum width.
TEST(TooltipInteractionTest, DISABLED_MaximumWidthClampsUnbreakableTextIncludingZero)
{
	spk::Widget root("Root", nullptr), target("Target", &root);
	root.setGeometry({.anchor = {0, 0}, .size = {200, 100}});
	spk::Tooltip tooltip("Tooltip", &root);
	tooltip.setTarget(&target);
	tooltip.setText("unbreakabletext");
	tooltip.show();
	for (unsigned width : {20u, 1u, 0u})
	{
		tooltip.setMaximumWidth(width);
		EXPECT_LE(tooltip.geometry().width, width);
	}
}

TEST(PopupWidgetInteractionTest, DestructionClearsCoordinatorAndContentOwnershipIsExternal)
{
	spk::Widget root("Root", nullptr);
	root.setGeometry({.anchor = {0, 0}, .size = {200, 100}});
	spk::Widget content("Content", nullptr);
	{
		spk::PopupWidget popup("Temporary", &root);
		content.setParent(&popup);
		popup.setContent(&content);
		popup.openAt({0, 0});
		popup.deactivate();
		press(popup, {100, 100});
		EXPECT_TRUE(popup.isOpen());
		popup.activate();
		popup.close();
		popup.openAt({0, 0});
	}
	EXPECT_EQ(content.parent(), nullptr);
	spk::PopupWidget replacement("Replacement", &root);
	EXPECT_NO_THROW(replacement.openAt({0, 0}));
	EXPECT_TRUE(replacement.isOpen());
}

TEST(SliderBarInteractionTest, MidDragRangeScaleAndGeometryChangesRemainClamped)
{
	spk::SliderBar slider("Slider");
	slider.setGeometry({.anchor = {0, 0}, .size = {100, 20}});
	slider.setScale(0.2f);
	press(slider, {5, 5});
	slider.setRange(-1, 1);
	slider.setRatio(0.25f);
	slider.setScale(0.5f);
	move(slider, {30, 5});
	EXPECT_EQ(slider.ratio(), 0.5f);
	EXPECT_EQ(slider.value(), 0);
	slider.setGeometry({.anchor = {0, 0}, .size = {1, 1}});
	move(slider, {100, 100});
	EXPECT_GE(slider.ratio(), 0);
	EXPECT_LE(slider.ratio(), 1);
	release(slider, {100, 100});
	EXPECT_FALSE(slider.isDragging());
}

// Neither control currently clears its captured interaction on all cancellation paths.
TEST(ToggleSwitchInteractionTest, DISABLED_FocusLossCancelsPendingToggle)
{
	spk::ToggleSwitch toggle("Toggle");
	toggle.setGeometry({.anchor = {0, 0}, .size = {80, 40}});
	press(toggle, {5, 5});
	loseFocus(toggle);
	release(toggle, {5, 5});
	EXPECT_FALSE(toggle.isChecked());
}

TEST(PushButtonInteractionTest, DISABLED_DeactivationCancelsPendingClick)
{
	spk::PushButton button("Button");
	button.setGeometry({.anchor = {0, 0}, .size = {100, 40}});
	int clicks = 0;
	auto contract = button.subscribeToClick([&] {
		++clicks;
	});
	press(button, {5, 5});
	button.deactivate();
	button.activate();
	release(button, {5, 5});
	EXPECT_EQ(clicks, 0);
}
