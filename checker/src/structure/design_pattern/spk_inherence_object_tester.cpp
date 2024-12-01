#include "structure/design_pattern/spk_inherence_object_tester.hpp"

TEST_F(InherenceObjectTest, AddChild)
{
    parentObject.addChild(&childObject1);

    ASSERT_EQ(parentObject.children().size(), 1) << "Parent should have 1 child";
    ASSERT_EQ(parentObject.children()[0], &childObject1) << "Child should be childObject1";
    ASSERT_EQ(childObject1.parent(), &parentObject) << "Child's parent should be parentObject";
}

TEST_F(InherenceObjectTest, RemoveChild)
{
    parentObject.addChild(&childObject1);
    parentObject.removeChild(&childObject1);

    ASSERT_EQ(parentObject.children().size(), 0) << "Parent should have 0 children after removal";
    ASSERT_EQ(childObject1.parent(), nullptr) << "Child's parent should be nullptr after removal";
}

TEST_F(InherenceObjectTest, TransferChildren)
{
    parentObject.addChild(&childObject1);
    parentObject.addChild(&childObject2);

    TestObject newParentObject;
    parentObject.transferChildren(&newParentObject);

    ASSERT_EQ(parentObject.children().size(), 0) << "Original parent should have 0 children after transfer";
    ASSERT_EQ(newParentObject.children().size(), 2) << "New parent should have 2 children after transfer";
    ASSERT_EQ(newParentObject.children()[0], &childObject1) << "First child should be childObject1";
    ASSERT_EQ(newParentObject.children()[1], &childObject2) << "Second child should be childObject2";
    ASSERT_EQ(childObject1.parent(), &newParentObject) << "ChildObject1's parent should be newParentObject";
    ASSERT_EQ(childObject2.parent(), &newParentObject) << "ChildObject2's parent should be newParentObject";
}

TEST_F(InherenceObjectTest, DestructorCleansUpParent)
{
    {
        TestObject tempParentObject;
        tempParentObject.addChild(&childObject1);
        tempParentObject.addChild(&childObject2);
    }

    ASSERT_EQ(childObject1.parent(), nullptr) << "ChildObject1's parent should be nullptr after parent's destruction";
    ASSERT_EQ(childObject2.parent(), nullptr) << "ChildObject2's parent should be nullptr after parent's destruction";
}

TEST_F(InherenceObjectTest, AddMultipleChildren)
{
    parentObject.addChild(&childObject1);
    parentObject.addChild(&childObject2);

    ASSERT_EQ(parentObject.children().size(), 2) << "Parent should have 2 children";
    ASSERT_EQ(parentObject.children()[0], &childObject1) << "First child should be childObject1";
    ASSERT_EQ(parentObject.children()[1], &childObject2) << "Second child should be childObject2";
    ASSERT_EQ(childObject1.parent(), &parentObject) << "ChildObject1's parent should be parentObject";
    ASSERT_EQ(childObject2.parent(), &parentObject) << "ChildObject2's parent should be parentObject";
}

TEST_F(InherenceObjectTest, RemoveNonExistentChild)
{
    parentObject.addChild(&childObject1);

    ASSERT_THROW(parentObject.removeChild(&childObject2), std::runtime_error) << "Removing of a non-inserted object should thorw an exception";

    ASSERT_EQ(parentObject.children().size(), 1) << "Parent should still have 1 child";
    ASSERT_EQ(parentObject.children()[0], &childObject1) << "Child should be childObject1";
    ASSERT_EQ(childObject1.parent(), &parentObject) << "ChildObject1's parent should still be parentObject";
    ASSERT_EQ(childObject2.parent(), nullptr) << "ChildObject2's parent should still be nullptr";
}