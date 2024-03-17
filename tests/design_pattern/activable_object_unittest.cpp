#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "design_pattern/spk_activable_object.hpp"

using namespace spk;

class ActivateObjectTest : public ::testing::Test {
protected:
    ActivateObject obj;
    int activationCount = 0;
    int deactivationCount = 0;

    virtual void SetUp() override {
		obj.deactivate();
        obj.setActivationCallback([this]() { activationCount++; });
        obj.setDeactivationCallback([this]() { deactivationCount++; });
		activationCount = 0;
		deactivationCount = 0;
	}		
};

TEST_F(ActivateObjectTest, InitialState) {
    ASSERT_FALSE(obj.isActive());
    ASSERT_EQ(activationCount, 0);
    ASSERT_EQ(deactivationCount, 0);
}

TEST_F(ActivateObjectTest, Activate) {
    obj.activate();
    ASSERT_TRUE(obj.isActive());
    ASSERT_EQ(activationCount, 1);
    ASSERT_EQ(deactivationCount, 0);
}

TEST_F(ActivateObjectTest, Deactivate) {
    obj.activate();
    ASSERT_EQ(activationCount, 1);

    obj.deactivate();
    ASSERT_FALSE(obj.isActive());
    ASSERT_EQ(deactivationCount, 1);
}

TEST_F(ActivateObjectTest, SwitchState) {
    ASSERT_FALSE(obj.isActive());
    ASSERT_EQ(activationCount, 0);
    
	obj.switchActivationState();
    ASSERT_TRUE(obj.isActive());
    ASSERT_EQ(activationCount, 1);

    obj.switchActivationState();
    ASSERT_FALSE(obj.isActive());
    ASSERT_EQ(deactivationCount, 1);
}

TEST_F(ActivateObjectTest, MultipleActivations) {
    obj.activate();
    obj.activate();
    ASSERT_EQ(activationCount, 1);

    obj.deactivate();
    obj.deactivate();
    ASSERT_EQ(deactivationCount, 1);
}