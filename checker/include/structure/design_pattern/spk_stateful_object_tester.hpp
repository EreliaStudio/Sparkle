#pragma once

#include <gtest/gtest.h>
#include "structure/design_pattern/spk_stateful_object.hpp"

class StatefulObjectTest : public ::testing::Test
{
protected:
    enum class State { State1, State2, State3 };

    int executionCount;
    spk::StatefulObject<State> statefulObject;

    StatefulObjectTest() : statefulObject(State::State1), executionCount(0) {}

    void SetUp() override
    {
        executionCount = 0;
    }

    void incrementCount()
    {
        executionCount++;
    }
};