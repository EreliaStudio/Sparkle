#pragma once

#include <gtest/gtest.h>

#include "structure/design_pattern/spk_observer.hpp"

class ObserverTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        notifier = new spk::Observer<std::string>();
        executionCount = 0;
    }

    void TearDown() override
    {
        delete notifier;
    }

    spk::Observer<std::string>* notifier;
    int executionCount = 0;
    spk::ContractProvider::Job incrementCountJob = [this]() { ++executionCount; };
};