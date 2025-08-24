#pragma once

#include <vector>

#include "structure/engine/spk_mesh.hpp"
#include "structure/math/spk_vector3.hpp"
#include "structure/engine/spk_obj_mesh.hpp"

namespace spk
{
	class CollisionMesh
	{
	public:
		struct Unit : public spk::TMesh<spk::Vector3>
		{
			using spk::TMesh<spk::Vector3>::TMesh;
		};

	private:
		std::vector<Unit> _units;

	public:
		CollisionMesh() = default;

		void addUnit(const Unit &p_unit);
		const std::vector<Unit> &units() const;

		static CollisionMesh fromObjMesh(const spk::SafePointer<spk::ObjMesh>& p_mesh);
	};
}
