#pragma once

#include "game_engine/component/spk_game_component.hpp"
#include "game_engine/spk_mesh.hpp"

namespace spk
{
	class ICollider : public spk::GameComponent
	{
	private:
		const Mesh* _mesh;

		void _onRender()
		{

		}

		void _onUpdate()
		{

		}

		virtual bool isIntersecting(const Mesh* p_mesh) = 0;

	public:
		ICollider(const std::string& p_name) :
			GameComponent(p_name)
		{

		}

		void setMesh(const Mesh* p_mesh)
		{
			_mesh = p_mesh;
		}
		const spk::Mesh* mesh() const
		{
			return (_mesh);
		}
	};
}