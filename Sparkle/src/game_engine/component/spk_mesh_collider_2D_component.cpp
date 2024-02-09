#include "game_engine/component/spk_mesh_collider_2D_component.hpp"

namespace spk
{
	bool MeshCollider2D::isIntersecting(const Mesh* p_mesh)
	{
		return (false);
	}

	MeshCollider2D::MeshCollider2D(const std::string& p_name) :
		ICollider(p_name)
	{

	}
}