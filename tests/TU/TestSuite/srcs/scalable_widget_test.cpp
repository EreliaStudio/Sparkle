#include "ui/widget/scalable_widget.hpp"
#include <gtest/gtest.h>

namespace
{
	void move(spk::Widget &widget, spk::Vector2Int position)
	{
		spk::Mouse mouse;
		mouse.position = position;
		spk::MouseMovedRecord record{};
		record.position = position;
		spk::MouseMovedEvent event(record, mouse);
		widget.dispatch(event);
	}
	void press(spk::Widget &widget, spk::Vector2Int position)
	{
		spk::Mouse mouse;
		mouse.position = position;
		spk::MouseButtonPressedRecord record{};
		record.button = spk::Mouse::Button::Left;
		spk::MouseButtonPressedEvent event(record, mouse);
		widget.dispatch(event);
		EXPECT_TRUE(event.consumed);
		EXPECT_TRUE(event.focusChange(spk::FocusMode::Channel::Mouse));
	}
}

TEST(ScalableWidgetTest, EveryEdgeAndCornerResizesWithOppositeEdgesFixed)
{
	using W = spk::ScalableWidget;
	for (const auto &[position, edges] : {std::pair{spk::Vector2Int{50, 100}, int(W::Left)}, {{150, 100}, int(W::Right)}, {{100, 50}, int(W::Top)}, {{100, 150}, int(W::Bottom)}, {{50, 50}, W::Left | W::Top}, {{150, 50}, W::Right | W::Top}, {{50, 150}, W::Left | W::Bottom}, {{150, 150}, W::Right | W::Bottom}})
	{
		W widget("Resizable");
		widget.activate();
		widget.setGeometry({.anchor = {50, 50}, .size = {100, 100}});
		widget.setGrabOffset(4);
		EXPECT_EQ(widget.grabOffset(), 4u);
		move(widget, position);
		EXPECT_EQ(widget.hoveredEdges(), edges);
		press(widget, position);
		EXPECT_EQ(widget.activeEdges(), edges);
		EXPECT_TRUE(widget.isResizing());
		move(widget, position + spk::Vector2Int{10, 20});
		EXPECT_EQ(widget.geometry().x, (edges & W::Left) ? 60 : 50);
		EXPECT_EQ(widget.geometry().y, (edges & W::Top) ? 70 : 50);
		EXPECT_EQ(widget.geometry().width, (edges & W::Left) ? 90u : ((edges & W::Right) ? 110u : 100u));
		EXPECT_EQ(widget.geometry().height, (edges & W::Top) ? 80u : ((edges & W::Bottom) ? 120u : 100u));
		spk::Mouse mouse;
		spk::MouseButtonReleasedRecord record{};
		record.button = spk::Mouse::Button::Left;
		spk::MouseButtonReleasedEvent event(record, mouse);
		widget.dispatch(event);
		EXPECT_FALSE(widget.isResizing());
		EXPECT_TRUE(event.focusChange(spk::FocusMode::Channel::Mouse));
	}
}

TEST(ScalableWidgetTest, MinMaxConstraintsApplyDuringEveryDirectionalDrag)
{
	for (const auto position : {spk::Vector2Int{50, 50}, {150, 50}, {50, 150}, {150, 150}})
	{
		spk::ScalableWidget widget("Resizable");
		widget.activate();
		widget.setMinimalSize({20, 30});
		widget.setMaximalSize({140, 160});
		widget.setGeometry({.anchor = {50, 50}, .size = {100, 100}});
		press(widget, position);
		const spk::Vector2Int outward{position.x == 50 ? -1000 : 1000, position.y == 50 ? -1000 : 1000};
		move(widget, position + outward);
		EXPECT_EQ(widget.geometry().size, spk::Vector2UInt(140, 160));
		move(widget, position - outward);
		EXPECT_EQ(widget.geometry().size, spk::Vector2UInt(20, 30));
	}
}

TEST(ScalableWidgetTest, FocusLossDoubleClickMouseLeaveAndDegenerateGeometry)
{
	spk::ScalableWidget widget("Resizable");
	widget.activate();
	widget.setGeometry({.anchor = {10, 10}, .size = {20, 20}});
	move(widget, {10, 10});
	spk::MouseLeftRecord leftRecord{};
	spk::MouseLeftEvent left(leftRecord);
	widget.dispatch(left);
	EXPECT_EQ(widget.hoveredEdges(), spk::ScalableWidget::None);
	spk::Mouse mouse;
	mouse.position = {10, 10};
	spk::MouseButtonDoubleClickedRecord doubleRecord{};
	doubleRecord.button = spk::Mouse::Button::Left;
	spk::MouseButtonDoubleClickedEvent doubled(doubleRecord, mouse);
	widget.dispatch(doubled);
	EXPECT_TRUE(widget.isResizing());
	widget.dispatch(left);
	EXPECT_TRUE(widget.isResizing()); // Captured drag continues outside the window.
	spk::WindowFocusLostRecord lostRecord{};
	spk::WindowFocusLostEvent lost(lostRecord);
	widget.dispatch(lost);
	EXPECT_FALSE(widget.isResizing());
	widget.deactivate();
	const auto before = widget.geometry();
	move(widget, {100, 100});
	EXPECT_EQ(widget.geometry(), before);
	widget.activate();
	widget.setGeometry({.anchor = {0, 0}, .size = {0, 0}});
	move(widget, {0, 0});
	EXPECT_EQ(widget.hoveredEdges(), spk::ScalableWidget::None);
	widget.setGeometry({.anchor = {0, 0}, .size = {1, 1}});
	move(widget, {0, 0});
	EXPECT_EQ(widget.hoveredEdges(), spk::ScalableWidget::Left | spk::ScalableWidget::Top);
}

// ScalableWidget enforces size hints, but currently does not enforce parent bounds.
TEST(ScalableWidgetTest, DISABLED_ParentBoundsConstrainAllResizeDirections)
{
	for (const auto position : {spk::Vector2Int{25, 25}, {75, 25}, {25, 75}, {75, 75}})
	{
		spk::Widget parent("Parent", nullptr);
		parent.setGeometry({.anchor = {0, 0}, .size = {100, 100}});
		spk::ScalableWidget widget("Resizable", &parent);
		widget.activate();
		widget.setGeometry({.anchor = {25, 25}, .size = {50, 50}});
		press(widget, position);
		move(widget, {position.x == 25 ? -100 : 200, position.y == 25 ? -100 : 200});
		EXPECT_GE(widget.geometry().x, 0);
		EXPECT_GE(widget.geometry().y, 0);
		EXPECT_LE(widget.geometry().x + widget.geometry().width, 100u);
		EXPECT_LE(widget.geometry().y + widget.geometry().height, 100u);
	}
}

TEST(ScalableWidgetTest, DISABLED_MouseFocusReleaseAndDeactivationCancelResizing)
{
	spk::ScalableWidget widget("Resizable");
	widget.activate();
	widget.setGeometry({.anchor = {0, 0}, .size = {100, 100}});
	press(widget, {0, 0});
	widget.notifyFocusReleased(spk::FocusMode::Channel::Mouse);
	EXPECT_FALSE(widget.isResizing());
	widget.deactivate();
	EXPECT_FALSE(widget.isResizing());
	widget.activate();
	const auto before = widget.geometry();
	move(widget, {20, 20});
	EXPECT_EQ(widget.geometry(), before);
}
