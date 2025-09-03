#pragma once

#include "structure/engine/spk_obj_mesh.hpp"
#include "structure/math/spk_polygon.hpp"
#include "structure/math/spk_vector3.hpp"
#include <vector>

namespace spk
{
	class CollisionMesh
	{
	public:
		using Unit = spk::Polygon;

	private:
		std::vector<Unit> _units;
		spk::BoundingBox _boundingBox;

	public:
		CollisionMesh() = default;

		void addUnit(const Unit &p_unit);
		const std::vector<Unit> &units() const;

		const spk::BoundingBox& boundingBox() const;

		static CollisionMesh fromObjMesh(const spk::SafePointer<spk::ObjMesh> &p_mesh);
	};
}
