#include "structure/engine/spk_mesh_renderer.hpp"

#include "structure/engine/spk_game_object.hpp"

namespace spk
{
	void MeshRenderer::onPaintEvent(spk::PaintEvent &p_event)
	{
		if (_mesh == nullptr)
		{
			return;
		}

		//_object.render();
	}

	void MeshRenderer::awake()
	{
		_onTransformEditionContract = owner()->transform().addOnEditionCallback(
			[&]()
			{
				// _modelMatrixElement = owner()->transform().model();
				// _modelInverseMatrixElement = owner()->transform().inverseModel();
				// _modelInformations.validate();
			});
		_onTransformEditionContract.trigger();
	}

	void MeshRenderer::sleep()
	{
		_onTransformEditionContract.resign();
	}

	MeshRenderer::MeshRenderer(const std::wstring &p_name) :
		spk::Component(p_name)
	{
	}

	void MeshRenderer::setMesh(spk::SafePointer<spk::Mesh> p_mesh)
	{
		_mesh = p_mesh;
		_onMeshValidationContract = p_mesh->subscribeToValidation(
			[&]()
			{
				// _object.layout().clear();
				// _object.indexes().clear();

				// _object.layout() << _mesh->vertices();
				// _object.indexes() << _mesh->indexes();

				// _object.layout().validate();
				// _object.indexes().validate();
			});
	}

	void MeshRenderer::setMaterial(spk::SafePointer<spk::Material> p_material)
	{
		_material = p_material;

		_onMaterialValidationContract = p_material->subscribeToValidation(
			[&]()
			{
				// _modelDiffuseTexture.bind(_material->diffuse());
				// _modelSpecularTexture.bind(_material->specular());

				// _modelMaterialElement[L"hasDiffuseTexture"] = static_cast<int>(_material->diffuse() != nullptr);
				// _modelMaterialElement[L"hasSpecularTexture"] = static_cast<int>(_material->specular() != nullptr);
				// _modelMaterialElement[L"shininess"] = _material->shininess();

				// _modelInformations.validate();
			});
	}

	spk::SafePointer<spk::Mesh> MeshRenderer::mesh() const
	{
		return (_mesh);
	}

	spk::SafePointer<spk::Material> MeshRenderer::material() const
	{
		return (_material);
	}
}