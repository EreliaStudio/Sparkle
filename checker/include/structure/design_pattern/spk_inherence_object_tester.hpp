#pragma once

#include <gtest/gtest.h>
#include "structure/design_pattern/spk_inherence_object.hpp"

class TestObject : public spk::InherenceObject<TestObject> {};

class InherenceObjectTest : public ::testing::Test
{
protected:
    TestObject parentObject;
    TestObject childObject1;
    TestObject childObject2;

    void SetUp() override
    {

    }

    void TearDown() override
    {

    }
};

