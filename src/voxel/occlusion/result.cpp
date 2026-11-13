#include "voxel/occlusion/result.hpp"

#include <utility>

namespace spk::Voxel::Occlusion
{
	Result::Result() :
		_fragments(std::make_shared<const Data>())
	{
	}

	Result::Result(Data fragments) :
		_fragments(std::make_shared<const Data>(std::move(fragments)))
	{
	}

	const Result::Data &Result::fragments() const noexcept
	{
		return *_fragments;
	}

	bool Result::empty() const noexcept
	{
		return _fragments->empty();
	}
}
