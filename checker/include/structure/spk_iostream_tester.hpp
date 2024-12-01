#pragma once

#include <gtest/gtest.h>
#include <sstream>
#include "structure/spk_iostream.hpp"


class IOStreamTest : public ::testing::Test
{
protected:
    std::wstringstream testStream1;
    spk::IOStream ioStream;

    IOStreamTest() : ioStream(testStream1) {}

    void SetUp() override
    {
        // Reset stream state before each test
        testStream1.str(L"");
        testStream1.clear();
    }
};