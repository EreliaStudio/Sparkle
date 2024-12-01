#pragma once

#include <gtest/gtest.h>

#include "structure/design_pattern/spk_event_notifier.hpp"

class EventNotifierTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        notifier = new spk::EventNotifier<std::string>();
        executionCount = 0;
    }

    void TearDown() override
    {
        delete notifier;
    }

    spk::EventNotifier<std::string>* notifier;
    int executionCount = 0;
    spk::ContractProvider::Job incrementCountJob = [this]() { ++executionCount; };
};