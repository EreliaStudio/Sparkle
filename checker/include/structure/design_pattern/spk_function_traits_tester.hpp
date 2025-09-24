#pragma once

#include <gtest/gtest.h>
#include <string>
#include <tuple>
#include <type_traits>

#include "structure/design_pattern/spk_function_traits.hpp"

struct TraitsTestFunctor
{
	int operator()(double, float) const
	{
		return 0;
	}
};

struct TraitsMutableFunctor
{
	void operator()(int)
	{
	}
};

struct TraitsNoexceptFunctor
{
	void operator()(int, const std::string &) const noexcept
	{
	}
};
