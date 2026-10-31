#pragma once

#include <memory>
#include <vector>

#include "voxel/shape.hpp"

namespace spk::Voxel::Occlusion
{
	class Result final
	{
	public:
		using Data = std::vector<Shape::Polygon>;

	private:
		std::shared_ptr<const Data> _fragments;

	public:
		Result();
		explicit Result(Data fragments);

		[[nodiscard]] const Data &fragments() const noexcept;
		[[nodiscard]] bool empty() const noexcept;
	};
}
