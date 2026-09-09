#pragma once

#include "query/union.hpp"

namespace spk
{
	template <typename TType, typename TContext>
		requires Hashable<TContext>
	using From = Union<TType, TContext>;
}
