#include "structure/engine/spk_mesh_renderer_2d.hpp"

#include "structure/engine/spk_game_object.hpp"

namespace spk
{
	MeshRenderer2D::MeshRenderer2D(const std::wstring &p_name) :
		spk::Component(p_name)
	{
	}

	void MeshRenderer2D::setMesh(spk::SafePointer<spk::Mesh2D> p_mesh)
	{
		_mesh2D = p_mesh;
		_onMeshValidationContract = p_mesh->subscribeToValidation(
			[&]()
			{
				// _object.layout().clear();
				// _object.indexes().clear();

				// _object.layout() << _mesh2D->vertices();
				// _object.indexes() << _mesh2D->indexes();

				// _object.layout().validate();
				// _object.indexes().validate();
			});
	}

	spk::SafePointer<spk::Mesh2D> MeshRenderer2D::mesh() const
	{
		return _mesh2D;
	}

	void MeshRenderer2D::setMaterial(spk::SafePointer<spk::Material> p_material)
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

	spk::SafePointer<spk::Material> MeshRenderer2D::material() const
	{
		return _material;
	}

	void MeshRenderer2D::onPaintEvent(spk::PaintEvent &p_event)
	{
		if (_mesh2D == nullptr)
		{
			return;
		}

		// _object.render();
	}

	void MeshRenderer2D::awake()
	{
		_onTransformEditionContract = owner()->transform().addOnEditionCallback(
			[&]()
			{
				// _modelMatrix = owner()->transform().model();
				// _modelInverseMatrix = owner()->transform().inverseModel();
				// _modelInformations.validate();
			});

		_onTransformEditionContract.trigger();
	}

	void MeshRenderer2D::sleep()
	{
		_onTransformEditionContract.resign();
	}
}
