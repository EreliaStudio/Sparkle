#pragma once

#include "voxel/model.hpp"
#include "voxel/occlusion/cache.hpp"
#include "voxel/volume.hpp"

namespace spk::Voxel
{
	class VoxelMesher
	{
	private:
		Occlusion::Cache *_cache = nullptr;
		float _tolerance = 1.0e-5f;

	protected:
		struct OutsideCell
		{
			enum class State
			{
				Empty,
				Occupied,
				Unknown
			};

			State state = State::Empty;
			Cell cell{};
		};

		[[nodiscard]] virtual OutsideCell outsideCell(
			spk::Vector3UInt position,
			Shape::Face direction) const;

	public:
		virtual ~VoxelMesher() = default;

		void bind(Occlusion::Cache *cache) noexcept;
		void setTolerance(float tolerance);
		[[nodiscard]] float tolerance() const noexcept;
		[[nodiscard]] Model bake(const Volume &volume) const;
	};
}
