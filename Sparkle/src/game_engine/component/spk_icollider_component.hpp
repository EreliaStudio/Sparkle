#pragma once

#include "game_engine/component/spk_game_component.hpp"
#include "game_engine/spk_mesh.hpp"
#include "game_engine/spk_game_object.hpp"

namespace spk
{
	class ICollider : public spk::GameComponent
	{
	private:
		const Mesh* _mesh;

		std::unique_ptr<Notifier::Contract> _translationContract;
		std::unique_ptr<Notifier::Contract> _scaleContract;
		std::unique_ptr<Notifier::Contract> _rotationContract;

		void _onRender()
		{

		}

		void _onUpdate()
		{

		}

		virtual void _onMeshEdition() = 0;

	public:
		ICollider(const std::string& p_name) :
			GameComponent(p_name),
			_mesh(nullptr),
			_translationContract(owner()->transform().translation.subscribe([&](){_onMeshEdition();})),
			_scaleContract(owner()->transform().scale.subscribe([&](){_onMeshEdition();})),
			_rotationContract(owner()->transform().rotation.subscribe([&](){_onMeshEdition();}))
		{
			
		}

		virtual bool isIntersecting(const ICollider* p_otherCollider) = 0;

		void setMesh(const Mesh* p_mesh)
		{
			_mesh = p_mesh;
			_onMeshEdition();
		}
		const spk::Mesh* mesh() const
		{
			return (_mesh);
		}
	};
}