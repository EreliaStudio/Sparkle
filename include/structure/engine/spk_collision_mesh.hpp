#pragma once

#include <vector>

#include "structure/engine/spk_obj_mesh.hpp"
#include "structure/math/spk_polygon.hpp"

namespace spk
{
	class CollisionMesh
	{
	private:
		std::vector<spk::Polygon> _polygons;

	public:
		CollisionMesh() = default;

		void addPolygon(const spk::Polygon &p_polygon);
		const std::vector<spk::Polygon> &polygons() const;

		static CollisionMesh fromObjMesh(const spk::SafePointer<spk::ObjMesh> &p_mesh);
	};
}
