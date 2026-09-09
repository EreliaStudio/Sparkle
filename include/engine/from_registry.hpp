#pragma once

#include "engine/registry.hpp"
#include "query/union.hpp"

namespace spk
{
	class Engine;

	template <typename TType>
	class FromRegistry final : public Union<TType, Engine *>
	{
	public:
		FromRegistry() :
			Union<TType, Engine *>(Registry<TType, Engine *>::instance())
		{
		}
	};
}
