#pragma once

#include "structure/design_pattern/spk_contract_provider.hpp"

#include "structure/graphics/spk_pipeline.hpp"

#include "structure/engine/spk_component.hpp"

#include "structure/engine/spk_mesh.hpp"
#include "structure/engine/spk_material.hpp"

namespace spk
{
	class MeshRenderer : public spk::Component
	{
	private:
		static std::string _pipelineCode;
		static spk::Pipeline _pipeline;

		spk::Pipeline::Object _object;
		spk::Pipeline::Object::Attribute& _modelInformations;
		spk::Pipeline::Object::Attribute::Element& _modelMatrixElement;
		spk::Pipeline::Object::Attribute::Element& _modelInverseMatrixElement;
		spk::Pipeline::Object::Attribute::Element& _modelMaterialElement;
		spk::Pipeline::Object::Sampler2D& _modelDiffuseTexture;
		spk::Pipeline::Object::Sampler2D& _modelSpecularTexture;

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