#include <gtest/gtest.h>

#include <type_traits>

#include "ui/widget/spacer_widget.hpp"
#include "ui/widget/workspace.hpp"

namespace
{
	static_assert(spk::WorkspaceContent<spk::Widget>);
	static_assert(spk::WorkspaceContent<spk::SpacerWidget>);
}

TEST(WorkspaceTest, TypedContentMenuAndGeometryUseMenuOverContentLayout)
{
	spk::Workspace<spk::Widget> workspace("Workspace");
	workspace.menuBar().setHeight(30);
	workspace.setGeometry({.anchor = {10, 20}, .size = {300, 200}});
	EXPECT_EQ(workspace.content().parent(), &workspace);
	EXPECT_EQ(workspace.menuBar().parent(), &workspace);
	EXPECT_EQ(workspace.content().geometry(), (spk::Rect2D{.anchor = {0, 30}, .size = {300, 170}}));
	EXPECT_EQ(workspace.menuBar().geometry(), (spk::Rect2D{.anchor = {0, 0}, .size = {300, 200}}));
	EXPECT_GT(workspace.menuBar().absoluteZOrder(), workspace.content().absoluteZOrder());
	const auto &constant = std::as_const(workspace);
	EXPECT_EQ(&constant.content(), &workspace.content());
	EXPECT_EQ(&constant.menuBar(), &workspace.menuBar());
}

TEST(WorkspaceTest, SmallGeometryAndContentHintChangesAreClampedAndPropagated)
{
	spk::Workspace<spk::SpacerWidget> workspace("Workspace");
	workspace.menuBar().setHeight(50);
	workspace.content().setMinimalSize({80, 40});
	EXPECT_GE(workspace.minimalSize().x, 80.0f);
	EXPECT_GE(workspace.minimalSize().y, 90.0f);
	workspace.setGeometry({.anchor = {0, 0}, .size = {20, 30}});
	EXPECT_EQ(workspace.content().geometry().size, spk::Vector2UInt(20, 0));
	workspace.setGeometry({.anchor = {0, 0}, .size = {0, 0}});
	EXPECT_EQ(workspace.content().geometry().size, spk::Vector2UInt(0, 0));
}
