#pragma once

#include <cstdint>

#include "voxel/model.hpp"
#include "voxel/occlusion/cache.hpp"
#include "voxel/volume.hpp"

namespace spk::Voxel
{
	class Mesher
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
		virtual ~Mesher() = default;

		void attachCache(Occlusion::Cache &cache) noexcept;
		void detachCache() noexcept;
		void setTolerance(float tolerance);
		[[nodiscard]] float tolerance() const noexcept;
		[[nodiscard]] Geometry bake(const Volume &volume, std::uint32_t materialIndexOffset = 0) const;
		[[nodiscard]] static Geometry::SurfaceMesh offsetMaterialIndices(const Geometry::SurfaceMesh &mesh, std::uint32_t offset);
		[[nodiscard]] static Geometry offsetMaterialIndices(const Geometry &mesh, std::uint32_t offset);
	};
}
