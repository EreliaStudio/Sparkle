#pragma once

#include <gtest/gtest.h>
#include "structure/design_pattern/spk_activable_object.hpp"

class ActivableObjectTest : public ::testing::Test
{
protected:
    int activationCount;
    int deactivationCount;
    spk::ActivableObject activableObject;

    ActivableObjectTest() : activationCount(0), deactivationCount(0) {}

    void SetUp() override
    {
        activationCount = 0;
        deactivationCount = 0;
    }

    void incrementActivationCount()
    {
        activationCount++;
    }

    void incrementDeactivationCount()
    {
        deactivationCount++;
    }
};