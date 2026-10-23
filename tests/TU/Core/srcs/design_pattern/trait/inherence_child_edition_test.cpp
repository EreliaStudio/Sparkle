#include "design_pattern/trait/inherence_trait.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
	struct Node : spk::InherenceTrait<Node>
	{
		std::string name;
		bool failAdded = false;

		explicit Node(std::string value) :
			name(std::move(value))
		{
		}

	protected:
		void _onChildAdded(Node *) override
		{
			if (failAdded)
			{
				throw std::runtime_error("Hook failure");
			}
		}
	};

	using Edition = Node::ChildEdition;
}

TEST(InherenceChildEditionTest, AdditionAndRemovalSeeCommittedRelationships)
{
	Node parent("parent");
	Node child("child");
	std::vector<Edition> events;
	auto contract = parent.subscribeToChildEdition([&](Node *changed, Edition edition) {
		EXPECT_EQ(changed, &child);
		EXPECT_EQ(changed->parent(), edition == Edition::Added ? &parent : nullptr);
		EXPECT_EQ(parent.children().empty(), edition == Edition::Removed);
		events.push_back(edition);
	});
	child.setParent(parent);
	child.clearParent();
	EXPECT_EQ(events, (std::vector<Edition>{Edition::Added, Edition::Removed}));
}

TEST(InherenceChildEditionTest, ReparentingReportsRemovalAndAdditionAfterCommit)
{
	Node original("original");
	Node destination("destination");
	Node child("child");
	child.setParent(original);
	std::vector<std::string> events;
	auto oldContract = original.subscribeToChildEdition([&](Node *, Edition edition) {
		EXPECT_EQ(edition, Edition::Removed);
		EXPECT_EQ(child.parent(), &destination);
		EXPECT_TRUE(original.children().empty());
		EXPECT_EQ(destination.children().size(), 1u);
		events.push_back("old");
	});
	auto newContract = destination.subscribeToChildEdition([&](Node *, Edition edition) {
		EXPECT_EQ(edition, Edition::Added);
		EXPECT_EQ(child.parent(), &destination);
		events.push_back("new");
	});
	events.clear();
	child.setParent(destination);
	EXPECT_EQ(events, (std::vector<std::string>{"old", "new"}));
}

TEST(InherenceChildEditionTest, FirstSubscriberFailureDoesNotSuppressRemainingSubscribers)
{
	Node parent("parent");
	Node child("child");
	int invoked = 0;
	auto first = parent.subscribeToChildEdition([&](Node *, Edition) {
		++invoked;
		throw std::runtime_error("first");
	});
	auto second = parent.subscribeToChildEdition([&](Node *, Edition) {
		++invoked;
	});
	auto third = parent.subscribeToChildEdition([&](Node *, Edition) {
		++invoked;
		throw std::runtime_error("third");
	});
	try
	{
		child.setParent(parent);
		FAIL() << "Expected callback failure.";
	} catch (const std::runtime_error &exception)
	{
		EXPECT_STREQ(exception.what(), "first");
	}
	EXPECT_EQ(invoked, 3);
	EXPECT_EQ(child.parent(), &parent);
	EXPECT_EQ(parent.children().size(), 1u);
}

TEST(InherenceChildEditionTest, HookFailureStillNotifiesSubscribersAndPreservesRelationship)
{
	Node parent("parent");
	Node child("child");
	parent.failAdded = true;
	int notified = 0;
	auto contract = parent.subscribeToChildEdition([&](Node *, Edition) {
		++notified;
	});
	EXPECT_THROW(child.setParent(parent), std::runtime_error);
	EXPECT_EQ(notified, 1);
	EXPECT_EQ(child.parent(), &parent);
}

TEST(InherenceChildEditionTest, ExistingParentEditionSubscribersAllReceiveNotifications)
{
	Node parent("parent");
	Node child("child");
	int invoked = 0;
	auto first = child.subscribeToParentEdition([&](const Node *) {
		++invoked;
		throw std::runtime_error("parent callback failed");
	});
	auto second = child.subscribeToParentEdition([&](const Node *current) {
		EXPECT_EQ(current, &parent);
		++invoked;
	});
	EXPECT_THROW(child.setParent(parent), std::runtime_error);
	EXPECT_EQ(invoked, 2);
	EXPECT_EQ(child.parent(), &parent);
}

TEST(InherenceChildEditionTest, DestructionRemovesChildWithoutPropagatingCallbackException)
{
	Node parent("parent");
	int removed = 0;
	auto contract = parent.subscribeToChildEdition([&](Node *, Edition edition) {
		if (edition == Edition::Removed)
		{
			++removed;
			throw std::runtime_error("destructor notification");
		}
	});
	{
		Node child("child");
		child.setParent(parent);
	}
	EXPECT_EQ(removed, 1);
	EXPECT_TRUE(parent.children().empty());
}

TEST(InherenceChildEditionTest, DestroyedParentDetachesChildrenAndNotifiesAll)
{
	Node child("child");
	int notified = 0;
	auto contract = child.subscribeToParentEdition([&](const Node *parent) {
		++notified;
		if (parent == nullptr)
		{
			throw std::runtime_error("detachment during destruction");
		}
	});
	{
		Node parent("parent");
		child.setParent(parent);
	}
	EXPECT_EQ(child.parent(), nullptr);
	EXPECT_EQ(notified, 2);
}
