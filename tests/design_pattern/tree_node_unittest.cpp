#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "design_pattern/spk_tree_node.hpp"

using namespace spk;

class TestNode : public TreeNode<TestNode> {
public:
    int value;

    TestNode(int val = 0) : value(val) {}
};

class TreeNodeTest : public ::testing::Test {
protected:
    TestNode parentNode;
    TestNode childNode1;
    TestNode childNode2;
    TestNode childNode3;

    virtual void SetUp() override {
        parentNode = TestNode(1);
        childNode1 = TestNode(2);
        childNode2 = TestNode(3);
        childNode3 = TestNode(4);
    }
};

TEST_F(TreeNodeTest, AddChild) {
    parentNode.addChild(&childNode1);
    parentNode.addChild(&childNode2);

    ASSERT_EQ(parentNode.children().size(), 2);
    ASSERT_EQ(childNode1.parent(), &parentNode);
    ASSERT_EQ(childNode2.parent(), &parentNode);
}

TEST_F(TreeNodeTest, RemoveChild) {
    TestNode* expectedParent = nullptr;

    parentNode.addChild(&childNode1);
    parentNode.addChild(&childNode2);

    parentNode.removeChild(&childNode1);

    ASSERT_EQ(parentNode.children().size(), 1);
    ASSERT_EQ(parentNode.children().front(), &childNode2);
    ASSERT_EQ(childNode1.parent(), expectedParent);
}

TEST_F(TreeNodeTest, TransferChildren) {
    parentNode.addChild(&childNode1);
    parentNode.addChild(&childNode2);

    TestNode newParentNode;
    parentNode.transferChildrens(&newParentNode);

    ASSERT_EQ(parentNode.children().size(), 0);
    ASSERT_EQ(newParentNode.children().size(), 2);
    ASSERT_EQ(childNode1.parent(), &newParentNode);
    ASSERT_EQ(childNode2.parent(), &newParentNode);
}

TEST_F(TreeNodeTest, DestructorRemovesParentChildAssociation) {
    {
        TestNode localNode;
        parentNode.addChild(&localNode);
    }  // localNode is destructed here

    ASSERT_TRUE(parentNode.children().empty());
}

TEST_F(TreeNodeTest, SelfAssignmentProtection) {
    EXPECT_THROW(parentNode.addChild(&parentNode), std::runtime_error);
}

TEST_F(TreeNodeTest, AddingExistingChild) {
    parentNode.addChild(&childNode1);
    TestNode anotherParentNode;
    anotherParentNode.addChild(&childNode1);

    ASSERT_EQ(childNode1.parent(), &anotherParentNode);
    ASSERT_TRUE(parentNode.children().empty());
}
