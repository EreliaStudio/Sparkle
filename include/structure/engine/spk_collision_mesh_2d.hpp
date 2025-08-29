#pragma once

#include "structure/engine/spk_mesh_2d.hpp"
#include "structure/math/spk_polygon_2d.hpp"
#include <vector>

namespace spk
{
	class CollisionMesh2D
	{
	public:
		using Unit = spk::Polygon2D;

	private:
		std::vector<Unit> _units;

	public:
		CollisionMesh2D() = default;

		void addUnit(const Unit &p_unit);
		const std::vector<Unit> &units() const;

		static CollisionMesh2D fromMesh(const spk::SafePointer<const spk::Mesh2D> &p_mesh);
	};
}
