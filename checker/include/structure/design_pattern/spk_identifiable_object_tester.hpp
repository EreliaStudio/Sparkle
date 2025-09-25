#pragma once

#include <gtest/gtest.h>

#include "structure/design_pattern/spk_identifiable_object.hpp"

class TestIdentifiedObject : public spk::IdentifiedObject<TestIdentifiedObject>
{
public:
	using spk::IdentifiedObject<TestIdentifiedObject>::IdentifiedObject;
};

class IdentifiedObjectTest : public ::testing::Test
{
protected:
	void TearDown() override
	{
	}
};
