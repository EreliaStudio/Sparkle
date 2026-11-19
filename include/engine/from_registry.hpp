#pragma once

#include "engine/registry.hpp"
#include "query/union.hpp"

namespace spk
{
	class EngineHeadless;

	template <typename TType>
	class FromRegistry final : public Union<TType, EngineHeadless *>
	{
	public:
		FromRegistry() :
			Union<TType, EngineHeadless *>(Registry<TType, EngineHeadless *>::instance())
		{
		}
	};
}
