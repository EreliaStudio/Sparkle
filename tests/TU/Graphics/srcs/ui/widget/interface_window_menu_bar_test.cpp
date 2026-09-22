#include <gtest/gtest.h>

#include "ui/widget/interface_window.hpp"

namespace
{
	const spk::Widget::Style &defaultStyle()
	{
		return spk::Widget::defaultStyle.get();
	}
}

TEST(InterfaceWindowMenuBarTest, DefaultStateAndAccessors)
{
	spk::IInterfaceWindow::MenuBar bar("Menu");
	EXPECT_EQ(bar.margin(), 3u);
	EXPECT_TRUE(bar.isButtonEnabled(spk::IInterfaceWindow::MenuBar::Button::Minimize));
	EXPECT_TRUE(bar.isButtonEnabled(spk::IInterfaceWindow::MenuBar::Button::Maximize));
	EXPECT_TRUE(bar.isButtonEnabled(spk::IInterfaceWindow::MenuBar::Button::Close));
	const auto &constant = static_cast<const spk::IInterfaceWindow::MenuBar &>(bar);
	EXPECT_EQ(&constant.titleLabel(), &bar.titleLabel());
	EXPECT_EQ(&constant.minimizeButton(), &bar.minimizeButton());
	EXPECT_EQ(&constant.maximizeButton(), &bar.maximizeButton());
	EXPECT_EQ(&constant.closeButton(), &bar.closeButton());
}

TEST(InterfaceWindowMenuBarTest, PublicConfigurationApiRoundTrips)
{
	spk::IInterfaceWindow::MenuBar bar("Menu");
	bar.applyStyle(defaultStyle());
	bar.setTitle("Inspector");
	bar.setIconset(defaultStyle().iconset.get());
	bar.setButtonEnabled(spk::IInterfaceWindow::MenuBar::Button::Maximize, false);
	bar.setMargin(8);
	EXPECT_EQ(bar.titleLabel().text(), spk::Font::textFromUTF8("Inspector"));
	EXPECT_FALSE(bar.isButtonEnabled(spk::IInterfaceWindow::MenuBar::Button::Maximize));
	EXPECT_EQ(bar.margin(), 8u);
}

TEST(InterfaceWindowMenuBarTest, InvalidButtonAndNullIconsetAreRejected)
{
	spk::IInterfaceWindow::MenuBar bar("Menu");
	const auto invalid = static_cast<spk::IInterfaceWindow::MenuBar::Button>(999);
	EXPECT_THROW(bar.setButtonEnabled(invalid, true), std::logic_error);
	EXPECT_THROW((void)bar.isButtonEnabled(invalid), std::logic_error);
	EXPECT_THROW(bar.setIconset(nullptr), std::invalid_argument);
}
