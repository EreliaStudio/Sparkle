#include <gtest/gtest.h>

#include "design_pattern/trait/inherence_trait.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
	struct Node;

	struct NodeComparator
	{
		bool operator()(const Node *left, const Node *right) const;
	};

	struct Node : spk::InherenceTrait<Node, NodeComparator>
	{
		using Base = spk::InherenceTrait<Node, NodeComparator>;

		std::string name;
		int orderingKey = 0;
		int childAddedCount = 0;
		int childRemovedCount = 0;
		Node *lastAddedChild = nullptr;
		Node *lastRemovedChild = nullptr;

		explicit Node(std::string p_name, int p_orderingKey = 0) :
			Base(NodeComparator{}),
			name(std::move(p_name)),
			orderingKey(p_orderingKey)
		{
		}

	protected:
		void _onChildAdded(Node *child) override
		{
			++childAddedCount;
			lastAddedChild = child;
		}

		void _onChildRemoved(Node *child) override
		{
			++childRemovedCount;
			lastRemovedChild = child;
		}
	};

	bool NodeComparator::operator()(const Node *left, const Node *right) const
	{
		return left->orderingKey < right->orderingKey;
	}
}

TEST(InherenceTraitTest, StandardUsageBuildsReparentsSortsAndNotifiesHierarchy)
{
	Node firstParent("first-parent");
	Node secondParent("second-parent");
	Node high("high", 30);
	Node low("low", 10);
	Node middle("middle", 20);

	std::vector<const Node *> parentChanges;
	auto contract = middle.subscribeToParentEdition([&](const Node *parent) {
		parentChanges.push_back(parent);
	});

	firstParent.addChild(&high);
	firstParent.addChild(&low);
	middle.setParent(&firstParent);

	ASSERT_EQ(firstParent.children().size(), 3u);
	EXPECT_EQ(firstParent.children()[0], &low);
	EXPECT_EQ(firstParent.children()[1], &middle);
	EXPECT_EQ(firstParent.children()[2], &high);
	EXPECT_EQ(middle.parent(), &firstParent);
	EXPECT_TRUE(middle.hasParent());

	middle.setParent(&secondParent);

	EXPECT_EQ(middle.parent(), &secondParent);
	EXPECT_EQ(firstParent.children(), (Node::ChildrenContainer{&low, &high}));
	EXPECT_EQ(secondParent.children(), (Node::ChildrenContainer{&middle}));
	EXPECT_EQ(parentChanges, (std::vector<const Node *>{&firstParent, &secondParent}));

	EXPECT_EQ(firstParent.childAddedCount, 3);
	EXPECT_EQ(firstParent.childRemovedCount, 1);
	EXPECT_EQ(firstParent.lastRemovedChild, &middle);
	EXPECT_EQ(secondParent.childAddedCount, 1);
	EXPECT_EQ(secondParent.lastAddedChild, &middle);
}

TEST(InherenceTraitTest, DetachingToNullRemovesChildAndNotifies)
{
	Node parent("parent");
	Node child("child");

	int callbackCount = 0;
	const Node *lastParent = reinterpret_cast<const Node *>(1);
	auto contract = child.subscribeToParentEdition([&](const Node *newParent) {
		++callbackCount;
		lastParent = newParent;
	});

	child.setParent(&parent);
	child.setParent(nullptr);

	EXPECT_FALSE(child.hasParent());
	EXPECT_EQ(child.parent(), nullptr);
	EXPECT_TRUE(parent.children().empty());
	EXPECT_EQ(callbackCount, 2);
	EXPECT_EQ(lastParent, nullptr);
	EXPECT_EQ(parent.childRemovedCount, 1);
	EXPECT_EQ(parent.lastRemovedChild, &child);
}

TEST(InherenceTraitTest, RepeatedSameParentAssignmentIsANoOp)
{
	Node parent("parent");
	Node child("child");
	int callbackCount = 0;

	auto contract = child.subscribeToParentEdition([&](const Node *) {
		++callbackCount;
	});

	child.setParent(&parent);
	ASSERT_EQ(callbackCount, 1);
	ASSERT_EQ(parent.childAddedCount, 1);

	child.setParent(&parent);

	EXPECT_EQ(callbackCount, 1);
	EXPECT_EQ(parent.childAddedCount, 1);
	EXPECT_EQ(parent.children().size(), 1u);
}

TEST(InherenceTraitTest, NotifyOrderingChangeResortsParentChildren)
{
	Node parent("parent");
	Node first("first", 10);
	Node second("second", 20);
	Node third("third", 30);

	parent.addChild(&first);
	parent.addChild(&second);
	parent.addChild(&third);

	first.orderingKey = 40;
	first.notifyOrderingChange();

	EXPECT_EQ(
		parent.children(),
		(Node::ChildrenContainer{&second, &third, &first}));
}

TEST(InherenceTraitTest, ExplicitSortChildrenUsesCurrentComparatorKeys)
{
	Node parent("parent");
	Node first("first", 10);
	Node second("second", 20);

	parent.addChild(&first);
	parent.addChild(&second);

	first.orderingKey = 50;
	parent.sortChildren();

	EXPECT_EQ(parent.children(), (Node::ChildrenContainer{&second, &first}));
}

TEST(InherenceTraitTest, ParentEditionContractCanBeResigned)
{
	Node firstParent("first");
	Node secondParent("second");
	Node child("child");
	int callbackCount = 0;

	auto contract = child.subscribeToParentEdition([&](const Node *) {
		++callbackCount;
	});

	child.setParent(&firstParent);
	EXPECT_EQ(callbackCount, 1);

	contract.resign();
	child.setParent(&secondParent);

	EXPECT_EQ(callbackCount, 1);
	EXPECT_EQ(child.parent(), &secondParent);
}

TEST(InherenceTraitTest, AddingNullChildThrowsRuntimeError)
{
	Node parent("parent");

	EXPECT_THROW(parent.addChild(nullptr), std::runtime_error);
}

TEST(InherenceTraitTest, RemovingNullChildThrowsInvalidArgument)
{
	Node parent("parent");

	EXPECT_THROW(parent.removeChild(nullptr), std::invalid_argument);
}

TEST(InherenceTraitTest, RemovingObjectThatIsNotAChildThrowsLogicError)
{
	Node parent("parent");
	Node other("other");

	EXPECT_THROW(parent.removeChild(&other), std::logic_error);
}

TEST(InherenceTraitTest, RemovingAlreadyDetachedChildThrowsLogicError)
{
	Node parent("parent");
	Node child("child");

	parent.addChild(&child);
	parent.removeChild(&child);
	ASSERT_FALSE(child.hasParent());

	EXPECT_THROW(parent.removeChild(&child), std::logic_error);
}

TEST(InherenceTraitTest, CircularHierarchyThrowsLogicError)
{
	Node root("root");
	Node child("child");
	Node grandChild("grand-child");

	root.addChild(&child);
	child.addChild(&grandChild);

	EXPECT_THROW(grandChild.addChild(&root), std::logic_error);

	EXPECT_FALSE(root.hasParent());
	EXPECT_EQ(child.parent(), &root);
	EXPECT_EQ(grandChild.parent(), &child);
}

TEST(InherenceTraitTest, DestroyingChildRemovesItFromLivingParent)
{
	Node parent("parent");

	{
		Node child("child");
		parent.addChild(&child);
		ASSERT_EQ(parent.children().size(), 1u);
		ASSERT_EQ(parent.children().front(), &child);
	}

	EXPECT_TRUE(parent.children().empty());
	EXPECT_EQ(parent.childRemovedCount, 1);
}

TEST(InherenceTraitTest, DestroyingParentDetachesLivingChildren)
{
	Node child("child");

	{
		Node parent("parent");
		parent.addChild(&child);
		ASSERT_EQ(child.parent(), &parent);
	}

	EXPECT_FALSE(child.hasParent());
	EXPECT_EQ(child.parent(), nullptr);
}

TEST(InherenceTraitTest, DestroyingMiddleNodeDetachesFromParentAndOrphansChildren)
{
	Node root("root");
	Node leaf("leaf");

	{
		Node middle("middle");
		root.addChild(&middle);
		middle.addChild(&leaf);

		ASSERT_EQ(middle.parent(), &root);
		ASSERT_EQ(leaf.parent(), &middle);
	}

	EXPECT_TRUE(root.children().empty());
	EXPECT_FALSE(leaf.hasParent());
	EXPECT_EQ(leaf.parent(), nullptr);
}
