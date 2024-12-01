#pragma once

#include "gtest/gtest.h"

#include "structure/graphics/spk_color.hpp"

class ColorTest : public ::testing::Test
{
protected:
    spk::Color defaultColor;
    spk::Color intColor;
    spk::Color floatColor;
    spk::Color jsonColor;

    virtual void SetUp();
    virtual void TearDown();
};