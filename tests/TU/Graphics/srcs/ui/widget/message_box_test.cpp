#include <gtest/gtest.h>

#include <stdexcept>

#include "ui/widget/message_box.hpp"

TEST(MessageBoxTest, TextButtonsMinimumWidthAndComposedAccessorsRoundTrip)
{
	spk::MessageBox box("Message");
	box.setTitle("Notice");
	box.setText(U"Something happened");
	box.setMinimalWidth(260);
	auto &accept = box.addButton("accept", "Accept");
	EXPECT_EQ(box.text(), U"Something happened");
	EXPECT_EQ(box.minimalWidth(), 260u);
	EXPECT_EQ(box.nbButton(), 1u);
	EXPECT_EQ(&box.button("accept"), &accept);
	EXPECT_EQ(&box.textArea(), &box.messageContent().textArea());
	EXPECT_EQ(&box.commandPanel(), &box.messageContent().commandPanel());
	EXPECT_GE(box.minimumContentSize().x, 260.0f);
}

TEST(MessageBoxTest, DynamicButtonsAndCommandErrorsPreserveState)
{
	spk::MessageBox box("Message");
	box.addButton("one", "One");
	box.addButton("two", "Two");
	box.removeButton("one");
	EXPECT_EQ(box.nbButton(), 1u);
	box.addButton("one", "Again");
	EXPECT_THROW(box.addButton("one", "Duplicate"), std::invalid_argument);
	EXPECT_THROW((void)box.button("missing"), std::out_of_range);
	EXPECT_EQ(box.nbButton(), 2u);
	box.setText("");
	EXPECT_TRUE(box.text().empty());
}
