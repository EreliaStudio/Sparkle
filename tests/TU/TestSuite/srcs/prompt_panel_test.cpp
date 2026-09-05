#include <gtest/gtest.h>

#include <stdexcept>

#include "ui/widget/prompt_panel.hpp"

TEST(PromptPanelTest, MessageActionsPoliciesAndGeometryAreComposed)
{
	spk::PromptPanel panel("Prompt");
	panel.setMessage("Continue?");
	auto &yes = panel.addButton("yes", "Yes");
	auto &no = panel.addButton("no", "No");
	const spk::Layout::SizeSettings policy{
		spk::Layout::SizePolicy::Extend, spk::Layout::SizePolicy::Fixed};
	panel.setButtonSizePolicy(policy);
	panel.setButtonPadding({12, 6});
	panel.setGeometry({.anchor = {20, 30}, .size = {320, 180}});
	EXPECT_EQ(panel.message(), U"Continue?");
	EXPECT_EQ(panel.nbButton(), 2u);
	EXPECT_EQ(&panel.button("yes"), &yes);
	EXPECT_EQ(&panel.button("no"), &no);
	EXPECT_EQ(panel.buttonSizePolicy(), policy);
	EXPECT_EQ(panel.buttonPadding(), spk::Vector2UInt(12, 6));
	EXPECT_EQ(panel.background().geometry().size, spk::Vector2UInt(320, 180));
}

TEST(PromptPanelTest, DynamicMessageAndActionsUpdateHintsAndRemainReusable)
{
	spk::PromptPanel panel("Prompt");
	const auto emptyHint = panel.preferredSize();
	panel.setMessage(U"A deliberately longer prompt message");
	EXPECT_GT(panel.preferredSize().y, emptyHint.y);
	panel.addButton("action", "Action");
	EXPECT_EQ(panel.nbButton(), 1u);
	panel.removeButton("action");
	EXPECT_EQ(panel.nbButton(), 0u);
	panel.addButton("action", "Again");
	EXPECT_EQ(panel.nbButton(), 1u);
	panel.setMessage("");
	EXPECT_TRUE(panel.message().empty());
}

TEST(PromptPanelTest, CommandErrorsPropagateWithoutCorruptingActions)
{
	spk::PromptPanel panel("Prompt");
	panel.addButton("ok", "OK");
	EXPECT_THROW(panel.addButton("ok", "Duplicate"), std::invalid_argument);
	EXPECT_THROW((void)panel.button("missing"), std::out_of_range);
	panel.removeButton("missing");
	EXPECT_EQ(panel.nbButton(), 1u);
	EXPECT_EQ(panel.button("ok").releasedLabel().text(), U"OK");
}
