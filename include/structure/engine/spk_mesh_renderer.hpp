#pragma once

#include "structure/design_pattern/spk_contract_provider.hpp"

#include "structure/engine/spk_component.hpp"

#include "structure/engine/spk_mesh.hpp"
#include "structure/engine/spk_material.hpp"

namespace spk
{
	class MeshRenderer : public spk::Component
	{
	private:
		ContractProvider::Contract _onTransformEditionContract;

		bool _needMeshUpload;
		spk::SafePointer<spk::Mesh> _mesh;

		spk::SafePointer<spk::Material> _material;

	public:	
		MeshRenderer(const std::wstring& p_name);

		void setMesh(spk::SafePointer<spk::Mesh> p_mesh);
		void setMaterial(spk::SafePointer<spk::Material> p_material);

		spk::SafePointer<spk::Mesh> mesh() const;
		spk::SafePointer<spk::Material> material() const;

		void onPaintEvent(spk::PaintEvent& p_event);

		void awake();
		void sleep();
	};
}