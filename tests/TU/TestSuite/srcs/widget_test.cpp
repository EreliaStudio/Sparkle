#include <gtest/gtest.h>

#include "core/context/update_context.hpp"
#include "exception.hpp"
#include "rendering/render_command.hpp"
#include "sparkle_test.hpp"
#include "ui/widget.hpp"

namespace
{
	struct Probe : spk::Widget
	{
		std::vector<std::string> *log = nullptr;
		bool consume = false;
		bool failUpdate = false;
		bool failRender = false;
		int hintUpdates = 0;
		using Widget::Widget;
		void _updateState(spk::UpdateContext &) override
		{
			if (failUpdate)
			{
				throw std::runtime_error("original update");
			}
			if (log)
			{
				log->push_back(name());
			}
		}
		void _buildRenderSnapshot(spk::RenderSnapshot::Builder &) override
		{
			if (failRender)
			{
				throw std::runtime_error("original render");
			}
			if (log)
			{
				log->push_back(name());
			}
		}
		void _onKeyPressedEvent(spk::KeyPressedEvent &event) override
		{
			if (log)
			{
				log->push_back(name());
			}
			event.consumed = consume;
		}
		void _onPassiveKeyPressedEvent(spk::KeyPressedEvent &) override
		{
			if (log)
			{
				log->push_back(name());
			}
		}
		void _updateSizeHint() override
		{
			++hintUpdates;
			if (!children().empty())
			{
				setPreferredSize(children().front()->preferredSize());
			}
		}
	};
}

TEST(WidgetTest, HierarchyReparentingDestructionGeometryAndZCaches)
{
	Probe root("Root", nullptr), other("Other", nullptr), child("Child", &root), grandchild("Grandchild", &child);
	root.setGeometry({.anchor = {10, 20}, .size = {100, 80}});
	child.setGeometry({.anchor = {80, 60}, .size = {50, 40}});
	grandchild.setGeometry({.anchor = {5, 5}, .size = {30, 30}});
	EXPECT_EQ(grandchild.viewRegion().viewport.anchor, spk::Vector2Int(95, 85));
	EXPECT_EQ(grandchild.viewRegion().scissor.size, spk::Vector2UInt(15, 15));
	root.setZOrder(2);
	child.setZOrder(3);
	grandchild.setZOrder(4);
	EXPECT_EQ(grandchild.absoluteZOrder(), 9);
	root.setZOrder(10);
	EXPECT_EQ(grandchild.absoluteZOrder(), 17);
	other.setGeometry({.anchor = {100, 100}, .size = {200, 200}});
	other.setZOrder(20);
	child.setParent(&other);
	EXPECT_EQ(&grandchild.root(), &other);
	EXPECT_TRUE(root.children().empty());
	EXPECT_EQ(grandchild.absoluteZOrder(), 27);
	EXPECT_EQ(grandchild.viewRegion().viewport.anchor, spk::Vector2Int(185, 165));
	EXPECT_EQ(static_cast<const Probe &>(grandchild).root().name(), "Other");
	{
		Probe temporary("Temporary", nullptr);
		child.setParent(&temporary);
	}
	EXPECT_EQ(child.parent(), nullptr);
	EXPECT_EQ(&grandchild.root(), &child);
	EXPECT_EQ(grandchild.absoluteZOrder(), 7);
}

TEST(WidgetTest, ActiveBranchesUpdateAscendingAndDispatchDescendingWithConsumption)
{
	Probe root("Root", nullptr), low("Low", &root), high("High", &root);
	std::vector<std::string> log;
	for (auto *widget : {&root, &low, &high})
	{
		widget->activate();
		widget->log = &log;
	}
	low.setZOrder(1);
	high.setZOrder(2);
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	spk::UpdateContext context{.time = {}, .deltaTime = {}, .keyboard = keyboard, .mouse = mouse};
	root.updateState(context);
	EXPECT_EQ(log, (std::vector<std::string>{"Root", "Low", "High"}));
	log.clear();
	high.deactivate();
	root.updateState(context);
	EXPECT_EQ(log, (std::vector<std::string>{"Root", "Low"}));
	high.activate();
	high.consume = true;
	log.clear();
	spk::KeyPressedRecord record{};
	record.key = spk::Keyboard::A;
	spk::KeyPressedEvent event(record, keyboard);
	root.dispatch(event);
	EXPECT_TRUE(event.consumed);
	EXPECT_EQ(log, (std::vector<std::string>{"High"}));
	log.clear();
	root.observeKeyboard(event);
	EXPECT_EQ(log.size(), 3u);
	root.deactivate();
	log.clear();
	root.updateState(context);
	EXPECT_TRUE(log.empty());
	spk::KeyPressedEvent inactive(record, keyboard);
	root.dispatch(inactive);
	EXPECT_TRUE(log.empty());
	EXPECT_FALSE(inactive.consumed);
}

TEST(WidgetTest, SnapshotSkipsClippedInactiveBranchesAndPassInheritanceTracksReparenting)
{
	Probe root("Root", nullptr), child("Child", &root), clipped("Clipped", &root), other("Other", nullptr);
	std::vector<std::string> log;
	root.setGeometry({.anchor = {0, 0}, .size = {100, 100}});
	child.setGeometry({.anchor = {10, 10}, .size = {50, 50}});
	clipped.setGeometry({.anchor = {101, 0}, .size = {10, 10}});
	for (auto *widget : {&root, &child, &clipped})
	{
		widget->activate();
		widget->log = &log;
	}
	spk::RenderSnapshot::Builder builder;
	root.buildRenderSnapshot(builder);
	EXPECT_EQ(log, (std::vector<std::string>{"Root", "Child"}));
	root.setTargetRenderPass(spk::Widget::BackgroundKey);
	EXPECT_EQ(child.targetRenderPass().name, spk::Widget::BackgroundKey.name);
	child.setTargetRenderPass(spk::Widget::PopupKey);
	EXPECT_TRUE(child.hasTargetRenderPassOverride());
	child.setParent(&other);
	EXPECT_EQ(child.targetRenderPass().name, spk::Widget::PopupKey.name);
	other.setTargetRenderPass(spk::Widget::TooltipKey);
	child.inheritTargetRenderPass();
	EXPECT_FALSE(child.hasTargetRenderPassOverride());
	EXPECT_EQ(child.targetRenderPass().name, spk::Widget::TooltipKey.name);
}

TEST(WidgetTest, ChildHintsPropagateAndDetachedChildrenStopNotifyingAncestors)
{
	Probe root("Root", nullptr), child("Child", &root), leaf("Leaf", &child);
	root.activate();
	child.activate();
	leaf.activate();
	leaf.setPreferredSize({50, 30});
	EXPECT_EQ(root.preferredSize(), spk::Vector2(50, 30));
	EXPECT_GT(root.hintUpdates, 0);
	EXPECT_GT(child.hintUpdates, 0);
	leaf.setParent(nullptr);
	const int before = child.hintUpdates;
	leaf.setPreferredSize({80, 40});
	EXPECT_EQ(child.hintUpdates, before);
}

TEST(WidgetTest, HookExceptionsRetainAncestorNamesAndOriginalCause)
{
	Probe root("Root", nullptr), child("FailingChild", &root);
	root.activate();
	child.activate();
	root.setGeometry({.anchor = {0, 0}, .size = {100, 100}});
	child.setGeometry(root.geometry());
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	spk::UpdateContext context{.time = {}, .deltaTime = {}, .keyboard = keyboard, .mouse = mouse};
	for (bool rendering : {false, true})
	{
		child.failUpdate = !rendering;
		child.failRender = rendering;
		try
		{
			if (rendering)
			{
				spk::RenderSnapshot::Builder builder;
				root.buildRenderSnapshot(builder);
			}
			else
			{
				root.updateState(context);
			}
			FAIL() << "Expected contextual exception";
		} catch (const spk::Exception &error)
		{
			EXPECT_NE(std::string(error.what()).find("Root"), std::string::npos);
			EXPECT_NE(std::string(error.what()).find("FailingChild"), std::string::npos);
			ASSERT_NE(error.cause(), nullptr);
			try
			{
				std::rethrow_exception(error.cause());
			} catch (const std::runtime_error &cause)
			{
				EXPECT_EQ(cause.what(), std::string(rendering ? "original render" : "original update"));
			}
		}
	}
}
