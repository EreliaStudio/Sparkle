#pragma once

#include <gtest/gtest.h>

#include "structure/design_pattern/spk_observable_value.hpp"

class ObservableValueTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        value.set(0);
    }

    spk::ObservableValue<int> value;
    int executionCount = 0;
    spk::ContractProvider::Job incrementCountJob = [this]() { ++executionCount; };
};