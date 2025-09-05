#pragma once

#include "structure/engine/spk_mesh_2d.hpp"
#include "structure/math/spk_polygon_2d.hpp"
#include "structure/spk_safe_pointer.hpp"
#include <vector>

namespace spk
{
	class CollisionMesh2D
	{
	public:
		using Unit = spk::Polygon2D;

	private:
		std::vector<Unit> _units;
		spk::BoundingBox2D _boundingBox;

	public:
		CollisionMesh2D() = default;

		void clear();

		void addUnit(const Unit &p_unit);
		const std::vector<Unit> &units() const;

		const spk::BoundingBox2D &boundingBox() const;

		bool intersect(const CollisionMesh2D &p_other) const;

		static CollisionMesh2D fromMesh(const spk::SafePointer<const spk::Mesh2D> &p_mesh);
	};
}
