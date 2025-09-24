#pragma once

#include <gtest/gtest.h>

#include "structure/design_pattern/spk_cached_data.hpp"

class CachedDataTest : public ::testing::Test
{
protected:
	mutable int generatorCalls;
	spk::CachedData<int> intCache;

	CachedDataTest() :
		generatorCalls(0),
		intCache([this]() { return generateInt(); })
	{
	}

	int generateInt()
	{
		generatorCalls++;
		return generatorCalls * 10;
	}
};
