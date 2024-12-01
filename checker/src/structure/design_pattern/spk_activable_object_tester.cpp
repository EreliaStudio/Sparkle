#include "structure/design_pattern/spk_activable_object_tester.hpp"

TEST_F(ActivableObjectTest, InitialState)
{
    ASSERT_FALSE(activableObject.isActive()) << "Initial state should be inactive (false)";
}

TEST_F(ActivableObjectTest, Activate)
{
    activableObject.activate();
    ASSERT_TRUE(activableObject.isActive()) << "Object should be active after calling activate()";
}

TEST_F(ActivableObjectTest, Deactivate)
{
    activableObject.activate();
    ASSERT_TRUE(activableObject.isActive()) << "Object should be active after calling activate()";

    activableObject.deactivate();
    ASSERT_FALSE(activableObject.isActive()) << "Object should be inactive after calling deactivate()";
}

TEST_F(ActivableObjectTest, AddActivationCallback)
{
    auto contract = activableObject.addActivationCallback([this]() { incrementActivationCount(); });

    ASSERT_EQ(activationCount, 0) << "Activation count should be 0 before activation";

    activableObject.activate();

    ASSERT_EQ(activationCount, 1) << "Activation count should be 1 after activation";
}

TEST_F(ActivableObjectTest, AddDeactivationCallback)
{
    auto contract = activableObject.addDeactivationCallback([this]() { incrementDeactivationCount(); });

    ASSERT_EQ(deactivationCount, 0) << "Deactivation count should be 0 before deactivation";

    activableObject.activate();
    ASSERT_TRUE(activableObject.isActive()) << "Object should be active after calling activate()";

    activableObject.deactivate();

    ASSERT_EQ(deactivationCount, 1) << "Deactivation count should be 1 after deactivation";
}

TEST_F(ActivableObjectTest, ResignActivationCallback)
{
    auto contract = activableObject.addActivationCallback([this]() { incrementActivationCount(); });

    contract.resign();

    ASSERT_EQ(activationCount, 0) << "Activation count should be 0 before activation";

    activableObject.activate();

    ASSERT_EQ(activationCount, 0) << "Activation count should still be 0 after activation since callback was resigned";
}

TEST_F(ActivableObjectTest, ResignDeactivationCallback)
{
    auto contract = activableObject.addDeactivationCallback([this]() { incrementDeactivationCount(); });

    contract.resign();

    ASSERT_EQ(deactivationCount, 0) << "Deactivation count should be 0 before deactivation";

    activableObject.activate();
    ASSERT_TRUE(activableObject.isActive()) << "Object should be active after calling activate()";

    activableObject.deactivate();

    ASSERT_EQ(deactivationCount, 0) << "Deactivation count should still be 0 after deactivation since callback was resigned";
}

TEST_F(ActivableObjectTest, MultipleActivationCallbacks)
{
    auto contract1 = activableObject.addActivationCallback([this]() { incrementActivationCount(); });
    auto contract2 = activableObject.addActivationCallback([this]() { incrementActivationCount(); });

    ASSERT_EQ(activationCount, 0) << "Activation count should be 0 before activation";

    activableObject.activate();

    ASSERT_EQ(activationCount, 2) << "Activation count should be 2 after activation triggers both callbacks";
}

TEST_F(ActivableObjectTest, MultipleDeactivationCallbacks)
{
    auto contract1 = activableObject.addDeactivationCallback([this]() { incrementDeactivationCount(); });
    auto contract2 = activableObject.addDeactivationCallback([this]() { incrementDeactivationCount(); });

    activableObject.activate();
    ASSERT_TRUE(activableObject.isActive()) << "Object should be active after calling activate()";

    ASSERT_EQ(deactivationCount, 0) << "Deactivation count should be 0 before deactivation";

    activableObject.deactivate();

    ASSERT_EQ(deactivationCount, 2) << "Deactivation count should be 2 after deactivation triggers both callbacks";
}

TEST_F(ActivableObjectTest, DestructorResignsCallbacks)
{
    {
        auto contract = activableObject.addActivationCallback([this]() { incrementActivationCount(); });

        ASSERT_EQ(activationCount, 0) << "Activation count should be 0 before activation";

        activableObject.activate();

        ASSERT_EQ(activationCount, 1) << "Activation count should be 1 after activation";
    }

    activableObject.deactivate();  // Ensure the object is deactivated before checking
    activationCount = 0;  // Reset count for next test

    activableObject.activate();
    ASSERT_EQ(activationCount, 0) << "Activation count should be 0 after activation since callback was resigned when contract went out of scope";
}

TEST_F(ActivableObjectTest, DestructorButRelinquishedCallback)
{
    {
        auto contract = activableObject.addActivationCallback([this]() { incrementActivationCount(); });

        ASSERT_EQ(activationCount, 0) << "Activation count should be 0 before activation";

        activableObject.activate();

        ASSERT_EQ(activationCount, 1) << "Activation count should be 1 after activation";

        contract.relinquish();
    }

    activableObject.deactivate();  // Ensure the object is deactivated before checking
    activationCount = 0;  // Reset count for next test

    activableObject.activate();
    ASSERT_EQ(activationCount, 1) << "Activation count should be 1 after activation with the contract being relinquished to the provider";
}